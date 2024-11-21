#include "training.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <hls_math.h>
#include <iostream>

void train(
    feature_vector &feature,
    label_vector   &label,
    Tree &tree,
    Node_hbm *nodePool,
    hls::stream<unit_interval> &rngStream
    )
{
    #pragma HLS stable variable=nodePool
    NodeManager_initiation: NodeManager nodeManager;
    //#pragma HLS AGGREGATE variable=nodeManager.nodeBuffer compact=auto
    //#pragma HLS BIND_STORAGE variable=nodeBuffer type=RAM_2P
    //#pragma HLS ARRAY_PARTITION variable=nodeBuffer dim=1 block factor=4 //TODO figure out how to partition to have access to multiple lowerBounds and upperBounds at the "same" time
    
    nodeManager.fetch_root(tree.root, nodePool);

    uint8_t depth = 0;
    bool done = false;

    Tree_traversal: while(!done){
        if(depth >= MAX_DEPTH) break;
        parallel_prefetch_process(nodeManager, feature, rngStream, done, nodePool);
        if(!done){
            depth++;
            nodeManager.prepare_next_nodes();
        }
    }
}

void process_node(NodeManager &nodeManager, feature_vector &feature, hls::stream<unit_interval> &rngStream, bool &done, Node_hbm *nodePool)
{
    #pragma HLS INLINE OFF
    #pragma HLS PIPELINE
    auto currentNode = nodeManager.getCurrent();
    auto parentNode = nodeManager.getParent();

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
        auto newParent = nodeManager.createNewNode();
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
        nodeManager.save_node(currentNode, nodePool);

        auto newSibbling = nodeManager.createNewNode();
        newSibbling.splittime = currentNode.splittime;
        newSibbling.parent = newParent.idx;

        //TODO: Change to sampleMondrian block, for now it is a leaf
        newSibbling.leaf = true;
        nodeManager.save_node(newSibbling, nodePool);

        if(feature.data[newParent.feature] <= newParent.threshold){ //TODO: can be better for sure
            newParent.leftChild = newSibbling.idx;
            newParent.rightChild = currentNode.idx;
        }else{
            newParent.leftChild = currentNode.idx;
            newParent.rightChild = newSibbling.idx;
        }
        nodeManager.save_node(parentNode, nodePool);
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
        nodeManager.save_node(currentNode, nodePool);
        nodeManager.setNextDirection((feature.data[currentNode.feature] <= currentNode.threshold) ? LEFT : RIGHT);
        //End of tree
        if(currentNode.leaf) {
            done = true;
        }
    }
}

void parallel_prefetch_process(NodeManager &nodeManager, feature_vector &feature, hls::stream<unit_interval> &rngStream, bool &done, Node_hbm *nodePool)
{
    #pragma HLS PIPELINE
    #pragma HLS DEPENDENCE dependent=false type=intra
    nodeManager.prefetch_nodes(nodePool);
    process_node(nodeManager, feature, rngStream, done, nodePool);
}

void NodeManager::prefetch_nodes(Node_hbm *nodePool)
{
    #pragma HLS PIPELINE
    fetch_node_from_memory(leftChildAddress, m.getLeftChildNodeIdx(), nodePool);
    fetch_node_from_memory(rightChildAddress, m.getRightChildNodeIdx(), nodePool);
}

void NodeManager::fetch_node_from_memory(int nodeAddress, ap_uint<2> localIdx, Node_hbm *nodePool)
{
    #pragma HLS INLINE
    if (nodeAddress < 0 || nodeAddress >= MAX_NODES) return;
    memcpy(&nodeBuffer[localIdx], &nodePool[nodeAddress], sizeof(Node_hbm));
}

void NodeManager::save_node(Node_hbm &node, Node_hbm *nodePool)
{
    #pragma HLS INLINE
    memcpy(&nodePool[node.idx], &node, sizeof(Node_hbm));
}

void NodeManager::prepare_next_nodes()
{
    //Select new path
    m.traverse(nextDir);

    leftChildAddress = getCurrent().leftChild;
    rightChildAddress = getCurrent().rightChild;
}

void NodeManager::fetch_root(int &address, Node_hbm *nodePool)
{
    #pragma HLS INLINE OFF
    
    fetch_node_from_memory(address, m.getCurrentNodeIdx(), nodePool); 
    leftChildAddress = getCurrent().leftChild;
    rightChildAddress = getCurrent().rightChild;
}