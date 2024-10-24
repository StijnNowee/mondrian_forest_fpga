#include "training.hpp"
#include <hls_math.h>

extern "C" {

    void train(
        hls::stream<feature_vector> &featureStream,
        hls::stream<label_vector>   &labelStream,
        hls::stream<fixed_point> &rng,
        hls::stream<Tree> &treeInfoStream,
        hls::stream<Tree> &treeOutputStream,
        Node_hbm *nodePool
        )
    {
        #pragma HLS DATAFLOW

        if(!featureStream.empty() && !labelStream.empty()){
            if( nodeIndex >= MAX_NODES) return;
            feature_vector feature = featureStream.read();
            label_vector label = labelStream.read();
            Tree tree = treeInfoStream.read();
            if(nodePool[tree.root].leaf){
                createMondrianTree(&tree, feature, nodePool);
            }
            while(!nodePool[tree.currentNode].leaf){
                ExtendMondrianBlock(&tree, tree.currentNode, feature, rng, nodePool);
            }
            tree.currentNode = tree.root;
            treeOutputStream.write(tree);
        }
    }

    void createMondrianTree(Tree *tree, feature_vector &feature, Node_hbm *nodePool)
    {
        auto node = nodePool[nodeIndex++];
        node.nodeIndex = nodeIndex;
        node.leaf = true;
        for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
            node.upperBound[d] = feature.data[d];
            node.lowerBound[d] = feature.data[d];
        }
        tree->root = node.nodeIndex;
        tree->currentNode = node.nodeIndex;
    }

    void ExtendMondrianBlock(Tree *tree, int node, feature_vector &feature, hls::stream<fixed_point> &rng, Node_hbm *nodePool)
    {
        //Retreive from HBM
        Node_hbm localNode = nodePool[node];
        Node_hbm parentNode = nodePool[localNode.parent];
        Tree localTree = *tree;

        fixed_point el[FEATURE_COUNT_TOTAL], eu[FEATURE_COUNT_TOTAL], weights[FEATURE_COUNT_TOTAL];
        fixed_point lresult = 0, uresult = 0;
        fixed_point rate = 0;

        for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //Can be much more efficient, needs update later
            lresult = localNode.lowerBound[d] - feature.data[d];
            uresult = localNode.upperBound[d] - feature.data[d];
            if (lresult > 0){
                el[d] = lresult;
            }else{
                el[d] = 0;
            }
            if(uresult > 0){
                eu[d] = uresult;
            }else{
                eu[d] = 0;
            }
            weights[d] = el[d] + eu[d];
            rate += weights[d]; 
        }
        fixed_point E = -hls::log(rng.read()) / rate;
        fixed_point potentialSplitTime = parentNode.splittime + E;
        if(potentialSplitTime < localNode.splittime){      //Introduce split
            splitNode(nodePool, localNode, potentialSplitTime, rate, rng, weights, eu, el, feature);
        }else{  //Continue
            updateAndTraverse(&localTree, &localNode, &feature, eu, el);
        }
        //Write back to HBM
        *tree = localTree;
        nodePool[node] = localNode;
    }

    void splitNode(Node_hbm *nodePool, Node_hbm &currentNode, fixed_point &potentialSplitTime, fixed_point &totalBounds, hls::stream<fixed_point> &rng, fixed_point *weights, fixed_point *eu, fixed_point *el, feature_vector &feature)
    {
        #pragma HLS INLINE
        Node_hbm newParent = Node_hbm();
        newParent.nodeIndex = nodeIndex++;
        newParent.splittime = potentialSplitTime;
        newParent.leaf = false;

        
        //Sample split dimension
        fixed_point randomValue = rng.read() * totalBounds;
        fixed_point sum = 0;
        for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
            sum += weights[d];
            if(sum > randomValue){
                newParent.feature = d;
                break;
            }
        }
        //Sample split location
        newParent.threshold = currentNode.lowerBound[newParent.feature] + rng.read() * (currentNode.upperBound[newParent.feature - currentNode.lowerBound[newParent.feature]]);

        //Set correct boundaries
        for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
            if(el[d] != 0){
                newParent.lowerBound[d] = feature.data[d];
            }
            if(eu[d] !=0){
                newParent.upperBound[d] = feature.data[d];
            }
        }        

        //Update node structure
        Node_hbm oldParent = nodePool[currentNode.parent];
        if(oldParent.leftChild == currentNode.nodeIndex){
            oldParent.leftChild = newParent.nodeIndex;
        }else{
            oldParent.rightChild = newParent.nodeIndex;
        }
        currentNode.parent = newParent.nodeIndex;

        Node_hbm newSibbling = Node_hbm();
        newSibbling.leaf = true;
        newSibbling.splittime = currentNode.splittime;
        newSibbling.parent = newParent.nodeIndex;
        newSibbling.nodeIndex = nodeIndex++;

        //Decide what side the nodes should be with respect to the parent
        if(feature.data[newParent.feature] <= newParent.threshold){
            newParent.leftChild = newSibbling.nodeIndex;
            newParent.rightChild = currentNode.nodeIndex;
        }else{
            newParent.leftChild = currentNode.nodeIndex;
            newParent.rightChild = newSibbling.nodeIndex;
        }


        //Write new node to the nodePool
        nodePool[newParent.nodeIndex] = newParent;
        nodePool[newSibbling.nodeIndex] = newSibbling;
        nodePool[oldParent.nodeIndex] = oldParent;
        nodePool[currentNode.nodeIndex] = currentNode;
    }

    void updateAndTraverse(Tree *localTree, Node_hbm* localNode, feature_vector *feature, fixed_point *eu, fixed_point *el)
    {
        #pragma HLS INLINE
        //Update bounds
        for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
            if(el[d] != 0){
                localNode->lowerBound[d] = feature->data[d];
            }
            if(eu[d] !=0){
                localNode->upperBound[d] = feature->data[d];
            }
        }

        //End of tree
        if(localNode->leaf)
            return;
        
        //Select new path
        if(feature->data[localNode->feature] <= localNode->threshold){
            localTree->currentNode = localNode->leftChild;
        }else{
            localTree->currentNode = localNode->rightChild;
        }
    }
}