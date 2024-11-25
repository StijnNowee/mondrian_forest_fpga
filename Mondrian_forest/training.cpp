#include "training.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <hls_math.h>
#include <iostream>
#include <iterator>

void train(
    feature_vector &feature,
    label_vector   &label,
    Node_hbm *nodePool,
    hls::stream<unit_interval> &rngStream,
    hls::stream<int> &rootNodeStream
    )
{
    #pragma HLS DATAFLOW
    //#pragma HLS stable variable=nodePool
    NodeManager_initiation: NodeManager nodeManager = NodeManager();
 
    //#pragma HLS AGGREGATE variable=nodeManager.nodeBuffer compact=auto
    //#pragma HLS BIND_STORAGE variable=nodeBuffer type=RAM_2P
    //#pragma HLS ARRAY_PARTITION variable=nodeBuffer dim=1 block factor=4 //TODO figure out how to partition to have access to multiple lowerBounds and upperBounds at the "same" time

    #pragma HLS DEPENDENCE dependent=false type=inter variable=nodePool

    hls::stream<FetchRequest, 2> fetch_stream("fetch_stream"), pre_fetch_stream("pre_fetch_stream"), root_fetch_stream("root_fetch_stream");

    hls::stream<Direction, 2> fetch_done_stream("fetch_done_stream");

    hls::stream<ProcessNodes, 1> process_nodes_stream("process_nodes_stream");

     hls::stream<bool, 1> process_done_stream("process_done_stream");

    hls::stream<localNodeIdx, 3> save_address_stream("save_address_stream");

    //nodeManager.fetch_root(tree.root, fetch_done_stream, nodePool);
    streamMerger(root_fetch_stream, pre_fetch_stream, fetch_stream);

    nodeManager.controller(fetch_done_stream, process_nodes_stream, process_done_stream, rootNodeStream, root_fetch_stream);

    nodeManager.prefetch_nodes(fetch_stream, fetch_done_stream, nodePool);

    process_node(nodeManager, feature, rngStream, process_nodes_stream, pre_fetch_stream, save_address_stream, process_done_stream);
    
    nodeManager.save_node(save_address_stream, nodePool);
}

void streamMerger(hls::stream<FetchRequest> &in1, hls::stream<FetchRequest> &in2, hls::stream<FetchRequest> &out)
{
    if(!in1.empty()){
        std::cout << "Multiplex" << std::endl;
        out.write(in1.read());
    } else if(!in2.empty()){
        std::cout << "Multiplex" << std::endl;
        out.write(in2.read());
    }
}

void NodeManager::controller(hls::stream<Direction> &fetch_done_stream, hls::stream<ProcessNodes> &process_nodes_stream, hls::stream<bool> &process_done_stream, hls::stream<int> &rootNodeStream, hls::stream<FetchRequest> &root_fetch_stream)
{
    int root;
    ProcessNodes p;

    bool done = false;
    while(true){
    switch (state){
        case START:
        //std::cout << "START" << std::endl;
            root = rootNodeStream.read();
            //std::cout << "Root: " << root << std::endl;
            root_fetch_stream.write(FetchRequest{.localIdx=m.getCurrentNodeIdx(), .address=root});
            //std::cout << "waiting" << std::endl;
            //fetch_node_from_memory(root, m.getCurrentNodeIdx(), fetch_done_stream, nodePool);
            fetch_done_stream.read();
            //std::cout << "fetch done" << std::endl;
            p = {.parentNode = m.getParentNodeIdx(), .currentNode = m.getCurrentNodeIdx()};
            process_nodes_stream.write(p);
            //std::cout<< "written" << std::endl;
            state = WAIT_FETCH;
        break;
        case WAIT_FETCH:
        //std::cout << "WAIT FETCH" << std::endl;
            for(int i = 0; i < 2; i++){
                fetch_done_stream.read();
            }
            state = WAIT_PROCESS;
        break;
        case WAIT_PROCESS:
        //std::cout << "WAIT PROCESS" << std::endl;
            p = {.parentNode = m.getParentNodeIdx(), .currentNode = m.getCurrentNodeIdx()};
            done = process_done_stream.read();
            //std::cout << "Done: " << done << std::endl;
            if(!done){
                process_nodes_stream.write(p);
                state = WAIT_FETCH;
            }else{
                state = START;
            }
        break;
    }
    }


    
}

void process_node(NodeManager &nodeManager, feature_vector &feature, hls::stream<unit_interval> &rngStream, hls::stream<ProcessNodes> &process_nodes_stream, hls::stream<FetchRequest> &pre_fetch_stream, hls::stream<localNodeIdx> &save_address_stream, hls::stream<bool> &process_done_stream)
{
    #pragma HLS PIPELINE
    std::cout << "Process Start" << std::endl;
    ProcessNodes p = process_nodes_stream.read();

    auto currentNode = nodeManager.getNode(p.currentNode);
    auto parentNode = nodeManager.getNode(p.parentNode);
    
    pre_fetch_stream.write(FetchRequest{.localIdx=nodeManager.getLeftChildIdx(), .address=currentNode.leftChild});
    pre_fetch_stream.write(FetchRequest{.localIdx=nodeManager.getRightChildIdx(), .address=currentNode.rightChild});

    std::cout << "fetch requested" << std::endl;
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
        //nodeManager.save_node(currentNode, nodePool);

        auto newSibbling = nodeManager.createNewNode();
        newSibbling.splittime = currentNode.splittime;
        newSibbling.parent = newParent.idx;

        //TODO: Change to sampleMondrian block, for now it is a leaf
        newSibbling.leaf = true;
        //nodeManager.save_node(newSibbling, nodePool);

        if(feature.data[newParent.feature] <= newParent.threshold){ //TODO: can be better for sure
            newParent.leftChild = newSibbling.idx;
            newParent.rightChild = currentNode.idx;
        }else{
            newParent.leftChild = currentNode.idx;
            newParent.rightChild = newSibbling.idx;
        }
        //nodeManager.save_node(parentNode, nodePool);
        process_done_stream.write(true);
        std::cout << "split?" << std::endl;

    }else{
        for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: not very efficient
            if(e_l[d] != 0){
                currentNode.lowerBound[d] = feature.data[d];
            }
            if(e_u[d] !=0){
                currentNode.upperBound[d] = feature.data[d];
            }
        }
        save_address_stream.write(p.currentNode);
        //nodeManager.save_node(currentNode, nodePool);
        //nodeManager.update_node(nodePool);
        //End of tree
        std::cout << "CurrentNode idx: " << currentNode.idx << std::endl;
        if(currentNode.leaf || currentNode.idx > 50) { //TODO: Remove test ending
        std::cout << "End of tree" << std::endl;
            process_done_stream.write(true);
        }else{
            std::cout << "Traversing" << std::endl;
            nodeManager.traverse((feature.data[currentNode.feature] <= currentNode.threshold) ? LEFT : RIGHT);
            process_done_stream.write(false);
        }
    }
}


void NodeManager::prefetch_nodes(hls::stream<FetchRequest> &fetch_address_stream, hls::stream<Direction> &fetch_done_stream ,const Node_hbm *nodePool)
{
    FetchRequest request = fetch_address_stream.read();
    fetch_node_from_memory(request.address, request.localIdx, fetch_done_stream, nodePool);
}

void NodeManager::fetch_node_from_memory(int nodeAddress, localNodeIdx localIdx, hls::stream<Direction> &fetch_done_stream, const Node_hbm *nodePool)
{
    #pragma HLS INLINE OFF
    if (nodeAddress < 0 || nodeAddress >= MAX_NODES) return;
    nodeBuffer[localIdx] = nodePool[nodeAddress];
    fetch_done_stream.write(RIGHT);
}

void NodeManager::save_node(hls::stream<localNodeIdx> &save_address_stream, Node_hbm *nodePool)
{
    std::cout << "save node" << std::endl;
    localNodeIdx localIdx = save_address_stream.read();
    nodePool[nodeBuffer[localIdx].idx] = nodeBuffer[localIdx];
    //memcpy(&nodePool[node.idx], &node, sizeof(Node_hbm));
}

void NodeManager::fetch_root(int &address, hls::stream<Direction> &fetch_done_stream, const Node_hbm *nodePool)
{
    fetch_node_from_memory(address, m.getCurrentNodeIdx(), fetch_done_stream, nodePool); 
    fetch_done_stream.write(LEFT);

}