#include "training.hpp"
#include <hls_math.h>
#include <hls_task.h>

extern "C" {
    void train(
        feature_vector &feature,
        label_vector   &label,
        Tree &tree,
        Node_hbm *nodePool
        )
    {
        #pragma HLS DATAFLOW
        #pragma HLS stable variable=nodePool
        hls::stream<Node_hbm> nodeFetchStream("nodeFetchStream");
        hls::stream<Node_hbm> nodeSaveStream("nodeSaveStream");
        hls::stream<int> nodeRequestStream("nodeRequestStream");


        nodeRequestStream.write(tree.root);


        fetch_node(nodeRequestStream, nodeFetchStream, nodePool);
        process_node(nodeFetchStream, nodeSaveStream, nodeRequestStream);
        save_node(nodeSaveStream, nodePool);


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
    void fetch_node(hls::stream<int> &nodeRequestStream, hls::stream<Node_hbm> &nodeFetchStream, Node_hbm *nodePool)
    {
        #pragma HLS INLINE
            if(!nodeRequestStream.empty()){
                int nodeIdx = nodeRequestStream.read();
                Node_hbm fetchedNode = nodePool[nodeIdx];
                nodeFetchStream.write(fetchedNode);
            }
    }

    void process_node(hls::stream<Node_hbm> &nodeFetchStream, hls::stream<Node_hbm> &nodeSaveStream, hls::stream<int> &nodeRequestStream)
    {
        #pragma HLS INLINE
            auto node = nodeFetchStream.read();
            node.feature = 5;
            node.leftChild = 1;
            nodeSaveStream.write(node);
            if (node.idx < 100){
                nodeRequestStream.write(node.idx + 1);
            }
    }

    void save_node(hls::stream<Node_hbm> &nodeSaveStream, Node_hbm *nodePool)
    {
        #pragma HLS INLINE
            if(!nodeSaveStream.empty()){
                auto node = nodeSaveStream.read();
                nodePool[node.idx] = node;
            }
    }
}