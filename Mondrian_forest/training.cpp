#include "training.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <hls_math.h>
#include <iostream>

extern "C" {
    void train(
        feature_vector &feature,
        label_vector   &label,
        Tree &tree,
        Node_hbm *nodePool,
        hls::stream<unit_interval> &rngStream
        )
    {
        #pragma HLS stable variable=nodePool
        std::cout << "Train entry" << std::endl;
        Node_hbm nodeBuffer[4];
        //#pragma HLS BIND_STORAGE variable=nodeBuffer type=RAM_2P
        //#pragma HLS ARRAY_PARTITION variable=nodeBuffer dim=1 block factor=4 //TODO figure out how to partition to have access to multiple lowerBounds and upperBounds at the "same" time
        #pragma HLS AGGREGATE variable=nodeBuffer compact=auto



        const uint8_t maxDepth = 200;
        uint8_t depth = 0;
        bool done = false;

        NodeMap m;
        
        fetch_node_from_memory(tree.root, m.getCurrentNodeIdx(), nodePool, nodeBuffer);
        int leftChildAddress = nodeBuffer[m.getCurrentNodeIdx()].leftChild;
        int rightChildAddress = nodeBuffer[m.getCurrentNodeIdx()].rightChild;


        Tree_traversal: while(!done){
            if(depth >= 50) break;
            parallel_prefetch_process(m, nodeBuffer, leftChildAddress, rightChildAddress, nodePool, feature, rngStream, tree, done);
            if(!done){
                prepare_next_nodes(nodeBuffer, m, depth,leftChildAddress, rightChildAddress, feature);
            }
            //save_node(m.currentNodeIdx, nodePool, nodeBuffer);
        }
    }

    void prefetch_node(NodeMap &m, Node_hbm* nodeBuffer, int &leftChildAddress, int &rightChildAddress, Node_hbm *nodePool)
    {
        #pragma HLS INLINE OFF
        #pragma HLS PIPELINE
        #pragma HLS dependence variable=nodeBuffer inter false

        if(rightChildAddress != leftChildAddress){
            fetch_node_from_memory(leftChildAddress, m.getLeftChildNodeIdx(), nodePool, nodeBuffer);
            fetch_node_from_memory(rightChildAddress, m.getRightChildNodeIdx(), nodePool, nodeBuffer);
        }
    }

    void fetch_node_from_memory(int &nodeAddress, ap_uint<2> localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer)
    {
        #pragma HLS INLINE

        if (nodeAddress < 0 || nodeAddress >= 50) return;
        nodeBuffer[localNodeAddress] = nodePool[nodeAddress];
    }
        
    void update_node(ap_uint<2> &localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer)
    {
        auto node = nodeBuffer[localNodeAddress];
        nodePool[node.idx] = node;
    }

    void save_new_node(Node_hbm &newNode, Node_hbm *nodePool)
    {
        #pragma HLS INLINE
        nodePool[newNode.idx] = newNode;
    }

    void process_node(Node_hbm &currentNode, Node_hbm &parentNode, feature_vector &feature, hls::stream<unit_interval> &rngStream, Tree &tree, Node_hbm *nodePool, bool &done)
    {
        #pragma HLS INLINE OFF
        #pragma HLS PIPELINE
        std::cout << "Processing" << std::endl;
        unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
        rate rate = 0,  weights[FEATURE_COUNT_TOTAL];
        for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
            e_l[i] = (currentNode.lowerBound[i] > feature.data[i]) ? static_cast<unit_interval>(currentNode.lowerBound[i] - feature.data[i]) : unit_interval(0);
            e_u[i] = (feature.data[i] > currentNode.upperBound[i]) ? static_cast<unit_interval>(feature.data[i] - currentNode.upperBound[i]) : unit_interval(0);
            weights[i] = e_l[i] + e_u[i];
            rate += weights[i];
        }
        float E = -std::log(static_cast<float>(rngStream.read())) / static_cast<float>(rate); //TODO: change from log to hls::log
        if(parentNode.splittime + E < currentNode.splittime){ //TODO: current node is root, what is parent node. 
            //Create new Node
            Node_hbm newParent;
            newParent.idx = tree.getNextFreeIdx();
            newParent.splittime = parentNode.splittime + E;
            newParent.leaf = false;

            //Sample split dimension
            float randomValue = rngStream.read() * rate;
            float sum = 0;
            for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //This can be combined in the initial rate calculation.
                sum += static_cast<float>(weights[d]);
                if(sum > randomValue){
                    newParent.feature = d;
                    break;
                }
            }
            // Sample split location
            newParent.threshold = currentNode.lowerBound[newParent.feature] + rngStream.read() * (currentNode.upperBound[newParent.feature - currentNode.lowerBound[newParent.feature]]);

            //Update boundaries
            for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: Maybe create a dummy node? Which can be promoted to the newParent mode. Saves it from copying this data
                if(e_l[d] != 0){
                    newParent.lowerBound[d] = feature.data[d];
                }
                if(e_u[d] !=0){
                    newParent.upperBound[d] = feature.data[d];
                }
            }
            //Update node structure
            if(parentNode.leftChild == currentNode.idx){
                parentNode.leftChild = newParent.idx;
            }else{
                parentNode.rightChild = newParent.idx;
            }
            currentNode.parent = parentNode.idx;
            

            Node_hbm newSibbling;
            newSibbling.idx = tree.getNextFreeIdx();
            newSibbling.splittime = currentNode.splittime;
            newSibbling.parent = newParent.idx;

            //TODO: Change to sampleMondrian block, for now it is a leaf
            newSibbling.leaf = true;
            save_new_node(newSibbling, nodePool);

            if(feature.data[newParent.feature] <= newParent.threshold){ //TODO: can be better for sure
                newParent.leftChild = newSibbling.idx;
                newParent.rightChild = currentNode.idx;
            }else{
                newParent.leftChild = currentNode.idx;
                newParent.rightChild = newSibbling.idx;
            }
            save_new_node(parentNode, nodePool);
            bool done = true;

        }else{
            for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: not very efficient
                if(e_l[d] != 0){
                    currentNode.lowerBound[d] = feature.data[d];
                }
                if(e_u[d] !=0){
                    currentNode.upperBound[d] = feature.data[d];
                }
            }
            //End of tree
            if(currentNode.leaf) {
                done = true;
            }
        }
    }

    void prepare_next_nodes(Node_hbm *nodeBuffer, NodeMap &m, uint8_t &depth, int &leftChildAddress, int &rightChildAddress, feature_vector &feature)
    {
        #pragma HLS INLINE OFF
        //Select new path
        Direction dir = (feature.data[nodeBuffer[m.getCurrentNodeIdx()].feature] <= nodeBuffer[m.getCurrentNodeIdx()].threshold) ? LEFT : RIGHT;
        m.traverse(dir);

        leftChildAddress = nodeBuffer[m.getCurrentNodeIdx()].leftChild;
        rightChildAddress = nodeBuffer[m.getCurrentNodeIdx()].rightChild;
        depth++;
    }

    void parallel_prefetch_process(NodeMap &m, Node_hbm *nodeBuffer, int &leftChildAddress, int &rightChildAddress, Node_hbm *nodePool, feature_vector &feature, hls::stream<unit_interval> &rngStream, Tree &tree, bool &done)
    {
        #pragma HLS PIPELINE
        #pragma HLS dependence variable=nodeBuffer inter false
        prefetch_node(m, nodeBuffer, leftChildAddress, rightChildAddress, nodePool);
        process_node(nodeBuffer[m.getCurrentNodeIdx()], nodeBuffer[m.getParentNodeIdx()], feature, rngStream, tree, nodePool, done);
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