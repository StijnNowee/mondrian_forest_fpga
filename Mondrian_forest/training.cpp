#include "training.hpp"
#include <cassert>
#include <cstdint>
#include <hls_math.h>

extern "C" {
    void train(
        feature_vector &feature,
        label_vector   &label,
        Tree &tree,
        Node_hbm *nodePool
        )
    {
        #pragma HLS stable variable=nodePool

        Node_hbm nodeBuffer[4];
        #pragma HLS BIND_STORAGE variable=nodeBuffer type=RAM_2P
        #pragma HLS ARRAY_PARTITION variable=nodeBuffer dim=1 block factor=4
        #pragma HLS AGGREGATE variable=nodeBuffer compact=auto

        bool done = false;

        const uint8_t maxDepth = 200;
        uint8_t depth = 0;

        NodeMap m;
        
        fetch_node_from_memory(tree.root, m.getCurrentNodeIdx(), nodePool, nodeBuffer);
        int leftChildAddress = nodeBuffer[m.getCurrentNodeIdx()].leftChild;
        int rightChildAddress = nodeBuffer[m.getCurrentNodeIdx()].rightChild;


        Tree_traversal: while(!done){
            if(depth >= maxDepth) break;
            parallel_prefetch_process(m, nodeBuffer, leftChildAddress, rightChildAddress, nodePool);
            prepare_next_nodes(nodeBuffer, m, done, depth,leftChildAddress, rightChildAddress);
            //save_node(m.currentNodeIdx, nodePool, nodeBuffer);
        }
    }

    void prefetch_node(NodeMap &m, Node_hbm* nodeBuffer, int &leftChildAddress, int &rightChildAddress, Node_hbm *nodePool)
    {
        #pragma HLS INLINE OFF

        fetch_node_from_memory(leftChildAddress, m.getLeftChildNodeIdx(), nodePool, nodeBuffer);
        fetch_node_from_memory(rightChildAddress, m.getRightChildNodeIdx(), nodePool, nodeBuffer);
    }

    void fetch_node_from_memory(int &nodeAddress, ap_uint<2> localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer)
    {
        
        nodeBuffer[localNodeAddress] = nodePool[nodeAddress];
        nodeBuffer[localNodeAddress].idx = nodeAddress;
        nodeBuffer[localNodeAddress].leftChild = nodeAddress + 1;
        nodeBuffer[localNodeAddress].rightChild = nodeAddress + 1;
    }
        
    void save_node(ap_uint<2> &localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer)
    {
        auto node = nodeBuffer[localNodeAddress];
        nodePool[node.idx] = node;
    }

    void process_node(Node_hbm &currentNode, Node_hbm &parentNode)
    {
        #pragma HLS INLINE OFF
        currentNode.feature = 5;
        currentNode.leftChild = 1;
        for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
            currentNode.upperBound[i] = 0.5;
        }
    }

    void prepare_next_nodes(Node_hbm *nodeBuffer, NodeMap &m, bool &done, uint8_t &depth, int &leftChildAddress, int &rightChildAddress)
    {
        #pragma HLS INLINE OFF
        Direction dir = (depth < 10) ? LEFT : RIGHT;
        m.traverse(dir);
        //nextNode(tmpIdx, m);

        leftChildAddress = nodeBuffer[m.getCurrentNodeIdx()].leftChild;
        rightChildAddress = nodeBuffer[m.getCurrentNodeIdx()].rightChild;
        if(m.getCurrentNodeIdx() > 20) {
            done = true;
        }
        depth++;
    }

    void parallel_prefetch_process(
        NodeMap &m,
        Node_hbm *nodeBuffer,
        int &leftChildAddress,
        int &rightChildAddress,
        Node_hbm *nodePool
    ) {
        #pragma HLS PIPELINE
        #pragma HLS dependence variable=nodeBuffer inter false
        prefetch_node(m, nodeBuffer, leftChildAddress, rightChildAddress, nodePool);
        process_node(nodeBuffer[m.getCurrentNodeIdx()], nodeBuffer[m.getParentNodeIdx()]);
    }

}

    // void createMondrianTree(Tree *tree, feature_vector &feature, Node_hbm *nodePool, int &freeNodeID)
    // {
    //     #pragma HLS PIPELINE II=1
    //     auto node = nodePool[freeNodeID];
    //     node.nodeIndex = freeNodeID;
    //     node.leaf = true;
    //     #pragma HLS ARRAY_PARTITION variable=node.lowerBound complete dim=1
    //     #pragma HLS ARRAY_PARTITION variable=node.upperBound complete dim=1
    //     for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
    //         node.upperBound[d] = feature.data[d];
    //         node.lowerBound[d] = feature.data[d];
    //     }
    //     tree->root = node.nodeIndex;
    //     tree->currentNode = node.nodeIndex;
    //     freeNodeID = -1;
    // }

    // void ExtendMondrianBlock(Tree *tree, int node, feature_vector &feature, hls::stream<fixed_point> &rng, Node_hbm *nodePool, int &freeNodeID)
    // {
    //     #pragma HLS PIPELINE II=1
    //     //Retreive from HBM
    //     Node_hbm &localNode = nodePool[node];
    //     Node_hbm &parentNode = nodePool[localNode.parent];
    //     Tree localTree = *tree;

    //     #pragma HLS ARRAY_PARTITION variable=localNode.lowerBound complete dim=1
    //     #pragma HLS ARRAY_PARTITION variable=localNode.upperBound complete dim=1

    //     fixed_point el[FEATURE_COUNT_TOTAL], eu[FEATURE_COUNT_TOTAL], weights[FEATURE_COUNT_TOTAL];
    //     fixed_point lresult = 0, uresult = 0;
    //     fixed_point rate = 0;

    //     for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //Can be much more efficient, needs update later
    //         #pragma HLS PIPELINE II=1
    //         lresult = localNode.lowerBound[d] - feature.data[d];
    //         uresult = localNode.upperBound[d] - feature.data[d];
    //         if (lresult > 0){
    //             el[d] = lresult;
    //         }else{
    //             el[d] = 0;
    //         }
    //         if(uresult > 0){
    //             eu[d] = uresult;
    //         }else{
    //             eu[d] = 0;
    //         }
    //         weights[d] = el[d] + eu[d];
    //         rate += weights[d]; 
    //     }
    //     fixed_point E = -hls::log(rng.read()) / rate;
    //     fixed_point potentialSplitTime = parentNode.splittime + E;
    //     if(potentialSplitTime < localNode.splittime){      //Introduce split
    //         splitNode(nodePool, localNode, potentialSplitTime, rate, rng, weights, eu, el, feature, freeNodeID);
    //     }else{  //Continue
    //         updateAndTraverse(&localTree, localNode, &feature, eu, el);
    //     }
    //     //Write back to HBM
    //     *tree = localTree;
    //     //nodePool[node] = localNode;
    // }

    // void splitNode(Node_hbm *nodePool, Node_hbm &currentNode, fixed_point &potentialSplitTime, fixed_point &totalBounds, hls::stream<fixed_point> &rng, fixed_point *weights, fixed_point *eu, fixed_point *el, feature_vector &feature, int &freeNodeID)
    // {
    //     #pragma HLS INLINE
    //     Node_hbm newParent = Node_hbm();
    //     newParent.nodeIndex = freeNodeID;
    //     newParent.splittime = potentialSplitTime;
    //     newParent.leaf = false;

        
    //     //Sample split dimension
    //     fixed_point randomValue = rng.read() * totalBounds;
    //     fixed_point sum = 0;
    //     for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
    //         sum += weights[d];
    //         if(sum > randomValue){
    //             newParent.feature = d;
    //             break;
    //         }
    //     }
    //     //Sample split location
    //     newParent.threshold = currentNode.lowerBound[newParent.feature] + rng.read() * (currentNode.upperBound[newParent.feature - currentNode.lowerBound[newParent.feature]]);

    //     //Set correct boundaries
    //     for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
    //         if(el[d] != 0){
    //             newParent.lowerBound[d] = feature.data[d];
    //         }
    //         if(eu[d] !=0){
    //             newParent.upperBound[d] = feature.data[d];
    //         }
    //     }        

    //     //Update node structure
    //     Node_hbm oldParent = nodePool[currentNode.parent];
    //     if(oldParent.leftChild == currentNode.nodeIndex){
    //         oldParent.leftChild = newParent.nodeIndex;
    //     }else{
    //         oldParent.rightChild = newParent.nodeIndex;
    //     }
    //     currentNode.parent = newParent.nodeIndex;

    //     Node_hbm newSibbling = Node_hbm();
    //     newSibbling.leaf = true;
    //     newSibbling.splittime = currentNode.splittime;
    //     newSibbling.parent = newParent.nodeIndex;
    //     newSibbling.nodeIndex = freeNodeID + 1;

    //     //Decide what side the nodes should be with respect to the parent
    //     if(feature.data[newParent.feature] <= newParent.threshold){
    //         newParent.leftChild = newSibbling.nodeIndex;
    //         newParent.rightChild = currentNode.nodeIndex;
    //     }else{
    //         newParent.leftChild = currentNode.nodeIndex;
    //         newParent.rightChild = newSibbling.nodeIndex;
    //     }

    //     freeNodeID = -1;

    //     //Write new node to the nodePool
    //     nodePool[newParent.nodeIndex] = newParent;
    //     nodePool[newSibbling.nodeIndex] = newSibbling;
    //     nodePool[oldParent.nodeIndex] = oldParent;
    //     nodePool[currentNode.nodeIndex] = currentNode;
    // }

    // void updateAndTraverse(Tree *localTree, Node_hbm &localNode, feature_vector *feature, fixed_point *eu, fixed_point *el)
    // {
    //     #pragma HLS INLINE
    //     //Update bounds
    //     for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
    //         if(el[d] != 0){
    //             localNode.lowerBound[d] = feature->data[d];
    //         }
    //         if(eu[d] !=0){
    //             localNode.upperBound[d] = feature->data[d];
    //         }
    //     }

    //     //End of tree
    //     if(localNode.leaf)
    //         return;
        
    //     //Select new path
    //     if(feature->data[localNode.feature] <= localNode.threshold){
    //         localTree->currentNode = localNode.leftChild;
    //     }else{
    //         localTree->currentNode = localNode.rightChild;
    //     }
    // }