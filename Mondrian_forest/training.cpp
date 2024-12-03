#include "training.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <hls_math.h>
#include <iostream>
#include <iterator>

void train(
    hls::stream<feature_vector> &featureStream,
    Node_hbm *nodePool,
    hls::stream<unit_interval> &rngStream,
    hls::stream<int> &rootNodeStream
    )
{
    #pragma HLS DATAFLOW
    NodeManager nodeManager = NodeManager();
    
    NodeMap m = NodeMap();
    Node_hbm nodeBuffer[4] = {};

    feature_vector feature;


    hls::stream<FetchRequest, 3> fetch_stream("fetch_stream"), pre_fetch_stream("pre_fetch_stream"), root_fetch_stream("root_fetch_stream");

    hls::stream<Direction, 2> fetch_done_stream("fetch_done_stream");

    hls::stream<ProcessNodes, 2> process_nodes_stream("process_nodes_stream");

    hls::stream<bool, 1> process_done_stream("process_done_stream");

    hls::stream<localNodeIdx, 3> save_address_stream("save_address_stream");

    

    #pragma HLS AGGREGATE variable=nodeBuffer compact=auto
    #pragma HLS ARRAY_PARTITION variable=nodeBuffer dim=1 type=complete
    //#pragma HLS AGGREGATE variable=nodeManager.nodeBuffer compact=auto
    //#pragma HLS BIND_STORAGE variable=nodeBuffer type=RAM_2P
    //#pragma HLS ARRAY_PARTITION variable=nodeBuffer dim=1 block factor=4 //TODO figure out how to partition to have access to multiple lowerBounds and upperBounds at the "same" time

    #pragma HLS DEPENDENCE dependent=false type=inter variable=nodePool
    #pragma HLS DEPENDENCE dependent=false type=inter variable=nodeBuffer
    #pragma HLS DEPENDENCE dependent=false type=intra variable=nodeBuffer
    
    
    streamMerger(root_fetch_stream, pre_fetch_stream, fetch_stream);
    
    //#pragma HLS INLINE recursive
    

    prefetch_nodes(fetch_stream, fetch_done_stream, nodePool, nodeBuffer);
    process_node(nodeManager, m, feature, rngStream, process_nodes_stream, pre_fetch_stream, save_address_stream, process_done_stream);
    controller(m, fetch_done_stream, process_nodes_stream, process_done_stream, rootNodeStream, root_fetch_stream, featureStream, feature);
    save_node(save_address_stream, nodePool, nodeBuffer);
   
}

void streamMerger(hls::stream<FetchRequest> &in1, hls::stream<FetchRequest> &in2, hls::stream<FetchRequest> &out)
{
    #pragma HLS INTERFACE mode=ap_none port=return
    //#pragma HLS PIPELINE
    if(!in1.empty()){
        out.write(in1.read());
    } else if(!in2.empty()){
        out.write(in2.read());
    }
}

void controller(const NodeMap &m, hls::stream<Direction> &fetch_done_stream, hls::stream<ProcessNodes> &process_nodes_stream, hls::stream<bool> &process_done_stream, hls::stream<int> &rootNodeStream, hls::stream<FetchRequest> &root_fetch_stream, hls::stream<feature_vector> &featureStream, feature_vector &feature)
{
    #pragma HLS PIPELINE
    int root;
    ProcessNodes p;
    ControllerState state = START;
    bool done = false;
    while(true){
        switch (state){
            case START:
            if(!rootNodeStream.empty()){
                root = rootNodeStream.read();
                feature = featureStream.read();
                root_fetch_stream.write(FetchRequest{.localIdx=m.getCurrentIdx(), .address=root});
                state = START_WAIT;
            }
            break;
            case START_WAIT:
            if(!fetch_done_stream.empty()){
                fetch_done_stream.read();
                p = {.parentNode = m.getParentIdx(), .currentNode = m.getCurrentIdx()};
                process_nodes_stream.write(p);
                state = WAIT_FETCH;
            }
            break;
            case WAIT_FETCH:
            if(fetch_done_stream.size() == 2){
                fetch_done_stream.read();
                fetch_done_stream.read();
                state = WAIT_PROCESS;
            }   
            break;
            case WAIT_PROCESS:
                if(!process_done_stream.empty()){
                    p = {.parentNode = m.getParentIdx(), .currentNode = m.getCurrentIdx()};
                    done = process_done_stream.read();
                    if(!done){
                        std::cout << "GOTO WAIT FETCH" << std::endl;
                        process_nodes_stream.write(p);
                        state = WAIT_FETCH;
                    }else{
                        state = START;
                    }
                }
            break;
        }
    }


    
}

void process_node(NodeManager &nodeManager, NodeMap &m, feature_vector &feature, hls::stream<unit_interval> &rngStream, hls::stream<ProcessNodes> &process_nodes_stream, hls::stream<FetchRequest> &pre_fetch_stream, hls::stream<localNodeIdx> &save_address_stream, hls::stream<bool> &process_done_stream)
{
    #pragma HLS PIPELINE
    #pragma HLS INTERFACE mode=ap_none port=return

    if(process_nodes_stream.empty()) return;
    ProcessNodes p = process_nodes_stream.read();

    Node_hbm currentNode;//= nodeBuffer[p.currentNode];
    //Node_hbm parentNode;//= nodeBuffer[p.parentNode];

    currentNode.leftChild = 5;
    currentNode.rightChild = 10;
    currentNode.idx = 2;
    currentNode.leaf = false;
    currentNode.feature = 5;
    currentNode.threshold = 0.5;

    rngStream.read();
    
    pre_fetch_stream.write(FetchRequest{.localIdx=m.getLeftChildIdx(), .address=currentNode.leftChild});
    pre_fetch_stream.write(FetchRequest{.localIdx=m.getRightChildIdx(), .address=currentNode.rightChild});

    // unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
    // rate rate = 0,  weights[FEATURE_COUNT_TOTAL];
    // for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
    //     e_l[i] = (currentNode.lowerBound[i] > feature.data[i]) ? static_cast<unit_interval>(currentNode.lowerBound[i] - feature.data[i]) : unit_interval(0);
    //     e_u[i] = (feature.data[i] > currentNode.upperBound[i]) ? static_cast<unit_interval>(feature.data[i] - currentNode.upperBound[i]) : unit_interval(0);
    //     weights[i] = e_l[i] + e_u[i];
    //     rate += weights[i];
    // }
    // float E = -std::log(static_cast<float>(rngStream.read())) / static_cast<float>(rate); //TODO: change from log to hls::log
    // if(parentNode.splittime + E < currentNode.splittime){ //TODO: current node is root, what is parent node. 
    //     //Create new Node
    //     auto newParent = nodeManager.createNewNode();
    //     newParent.splittime = parentNode.splittime + E;
    //     newParent.leaf = false;

    //     //Sample split dimension
    //     float randomValue = rngStream.read() * rate;
    //     float sum = 0;
    //     for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //This can be combined in the initial rate calculation.
    //         sum += static_cast<float>(weights[d]);
    //         if(sum > randomValue){
    //             newParent.feature = d;
    //             break;
    //         }
    //     }
    //     // Sample split location
    //     newParent.threshold = currentNode.lowerBound[newParent.feature] + rngStream.read() * (currentNode.upperBound[newParent.feature - currentNode.lowerBound[newParent.feature]]);

    //     //Update boundaries
    //     for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: Maybe create a dummy node? Which can be promoted to the newParent mode. Saves it from copying this data
    //         if(e_l[d] != 0){
    //             newParent.lowerBound[d] = feature.data[d];
    //         }
    //         if(e_u[d] !=0){
    //             newParent.upperBound[d] = feature.data[d];
    //         }
    //     }
    //     //Update node structure
    //     if(parentNode.leftChild == currentNode.idx){
    //         parentNode.leftChild = newParent.idx;
    //     }else{
    //         parentNode.rightChild = newParent.idx;
    //     }
    //     currentNode.parent = parentNode.idx;
    //     //nodeManager.save_node(currentNode, nodePool);

    //     auto newSibbling = nodeManager.createNewNode();
    //     newSibbling.splittime = currentNode.splittime;
    //     newSibbling.parent = newParent.idx;

    //     //TODO: Change to sampleMondrian block, for now it is a leaf
    //     newSibbling.leaf = true;
    //     //nodeManager.save_node(newSibbling, nodePool);

    //     if(feature.data[newParent.feature] <= newParent.threshold){ //TODO: can be better for sure
    //         newParent.leftChild = newSibbling.idx;
    //         newParent.rightChild = currentNode.idx;
    //     }else{
    //         newParent.leftChild = currentNode.idx;
    //         newParent.rightChild = newSibbling.idx;
    //     }
    //     //nodeManager.save_node(parentNode, nodePool);
    //     process_done_stream.write(true);
    //     std::cout << "split?" << std::endl;

    // }else{
    //     for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: not very efficient
    //         if(e_l[d] != 0){
    //             currentNode.lowerBound[d] = feature.data[d];
    //         }
    //         if(e_u[d] !=0){
    //             currentNode.upperBound[d] = feature.data[d];
    //         }
    //     }
        //save_address_stream.write(p.currentNode);
        //nodeManager.save_node(currentNode, nodePool);
        //nodeManager.update_node(nodePool);
        //End of tree
        std::cout << "CurrentNode idx: " << currentNode.idx << std::endl;
        if(currentNode.leaf || currentNode.idx > 50) { //TODO: Remove test ending
        std::cout << "End of tree" << std::endl;
            process_done_stream.write(true);
        }else{
            m.traverse((feature.data[currentNode.feature] <= currentNode.threshold) ? LEFT : RIGHT);
            process_done_stream.write(false);
        }
   // }
}


void prefetch_nodes(hls::stream<FetchRequest> &fetch_address_stream, hls::stream<Direction> &fetch_done_stream ,const Node_hbm *nodePool, Node_hbm *nodeBuffer)
{
    #pragma HLS PIPELINE
    #pragma HLS INTERFACE mode=ap_none port=return
    if(!fetch_address_stream.empty()){
        FetchRequest request = fetch_address_stream.read();
        fetch_node_from_memory(request.address, request.localIdx, fetch_done_stream, nodePool, nodeBuffer);
    }
}

void fetch_node_from_memory(int nodeAddress, localNodeIdx localIdx, hls::stream<Direction> &fetch_done_stream, const Node_hbm *nodePool, Node_hbm *nodeBuffer)
{
    #pragma HLS INLINE
    if (nodeAddress < 0 || nodeAddress >= MAX_NODES) return;
    nodeBuffer[localIdx] = nodePool[nodeAddress];
    fetch_done_stream.write(RIGHT);
}

void save_node(hls::stream<localNodeIdx> &save_address_stream, Node_hbm *nodePool, const Node_hbm *nodeBuffer)
{
    #pragma HLS INTERFACE mode=ap_none port=return
    if(!save_address_stream.empty()){
        localNodeIdx localIdx = save_address_stream.read();
        nodePool[nodeBuffer[localIdx].idx] = nodeBuffer[localIdx];
    }
}