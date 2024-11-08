#include "training.hpp"
#include <hls_math.h>

extern "C" {
    void train(
        feature_vector &feature,
        label_vector   &label,
        Tree &tree,
        Node_hbm *nodePool
        )
    {
        #pragma HLS PIPELINE II=1
        #pragma HLS stable variable=nodePool
        hls::stream<ap_uint<1>> fetch_done("fetch_doneStream");

        enum State state = START;

        Node_hbm nodeBuffer[4];
        #pragma HLS BIND_STORAGE variable=nodeBuffer type=RAM_2P
        #pragma HLS ARRAY_PARTITION variable=nodeBuffer dim=1 block factor=4
        #pragma HLS AGGREGATE variable=nodeBuffer compact=auto

        bool done = false;
        bool left = false;

        NodeMap m;
        ap_uint<1> fetch;
        
        fetch_node_from_memory(tree.root, m.currentNodeIdx, nodePool, nodeBuffer);
        fetch_done.write(1);
        Tree_traversal: while(!done){
            switch (state){
                case START:
                if(fetch_done.read_nb(fetch)){
                    state = PROCESS;
                    prefetch_node(m, nodeBuffer, nodePool, fetch_done);
                }

                case PROCESS: 
                    process_node(nodeBuffer[m.currentNodeIdx]);

                    save_node(m.currentNodeIdx, nodePool, nodeBuffer);
                    if(nodeBuffer[m.currentNodeIdx].idx < 10){
                        left = false;
                    }else{
                        left = true;
                    }
                    if(nodeBuffer[m.currentNodeIdx].idx > 20){
                        done = true;
                    }
                    state = WAITFORFETCH;
                break;
                case WAITFORFETCH:
                    if(fetch_done.read_nb(fetch)){
                        ap_uint<2> tmpIdx = (left) ? m.leftChildNodeIdx : m.rightChildNodeIdx;
                        nextNode(tmpIdx, m);
                        prefetch_node(m, nodeBuffer, nodePool, fetch_done);
                        state = PROCESS;
                    }
                break;
            }
        }
    }

    void prefetch_node(NodeMap &m, Node_hbm *nodeBuffer, Node_hbm *nodePool, hls::stream<ap_uint<1>> &fetch_done)
    {
        //#pragma HLS DATAFLOW
        #pragma HLS INLINE OFF

        //hls::stream<ap_uint<1>> left_fetch_done("left_fetch_done");
        //hls::stream<ap_uint<1>> right_fetch_done("right_fetch_done");

        fetch_node_from_memory(nodeBuffer[m.currentNodeIdx].leftChild, m.leftChildNodeIdx, nodePool, nodeBuffer);
        fetch_node_from_memory(nodeBuffer[m.currentNodeIdx].rightChild, m.rightChildNodeIdx, nodePool, nodeBuffer);

        fetch_done.write(1);
        //wait_for_fetch(left_fetch_done, right_fetch_done, fetch_done);

    }

    void fetch_node_from_memory(int &nodeAddress, ap_uint<2> &localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer)
    {
        #pragma HLS INLINE OFF
        nodeBuffer[localNodeAddress] = nodePool[nodeAddress];
        nodeBuffer[localNodeAddress].idx = nodeAddress;
        nodeBuffer[localNodeAddress].leftChild = nodeAddress + 1;
        nodeBuffer[localNodeAddress].rightChild = nodeAddress + 1;
        //fetch_done.write(1);
    }
        
    void save_node(ap_uint<2> &localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer)
    {
        #pragma HLS INLINE OFF
        auto node = nodeBuffer[localNodeAddress];
        nodePool[node.idx] = node;
    }

    void nextNode(ap_uint<2> &nextNodeIdx, NodeMap &m)
    {
        #pragma HLS INLINE OFF
        ap_uint<2> oldParentIdx = m.parentNodeIdx;
        m.parentNodeIdx = m.currentNodeIdx;
        m.currentNodeIdx = nextNodeIdx;
        if(m.leftChildNodeIdx == nextNodeIdx){
            m.leftChildNodeIdx = oldParentIdx;
        }else{
            m.rightChildNodeIdx = oldParentIdx;
        }
    }

    void process_node(Node_hbm &currentNode)
    {
        #pragma HLS INLINE OFF
        currentNode.feature = 5;
        currentNode.leftChild = 1;
    }

    void wait_for_fetch(hls::stream<ap_uint<1>> &left_fetch_done, hls::stream<ap_uint<1>> &right_fetch_done, hls::stream<ap_uint<1>> &fetch_done)
    {
        ap_uint<1> leftReceived = false;
        ap_uint<1> rightReceived = false;

        Wait_for_fetch_loop: while(!leftReceived || !rightReceived){
            left_fetch_done.read_nb(leftReceived);
            right_fetch_done.read_nb(rightReceived);
        }

        fetch_done.write(1);
    }


}

        //treeOutputStream.write(tree);
        // if(nodePool[tree.root].leaf){
        //     createMondrianTree(&tree, feature, nodePool);
        // }
        // while(!nodePool[tree.currentNode].leaf){
        //     ExtendMondrianBlock(&tree, tree.currentNode, feature, rng, nodePool);
        // }
        // tree.currentNode = tree.root;
        // treeOutputStream.write(tree);

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