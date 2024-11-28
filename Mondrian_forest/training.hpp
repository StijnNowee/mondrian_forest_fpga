#ifndef TRAINING_HPP
#define TRAINING_HPP

#include "common.hpp"
#include <cstdint>

enum Direction{
    LEFT,
    RIGHT
};

struct NodeMap {
    private:
    localNodeIdx parentNodeIdx = 3;
    localNodeIdx currentNodeIdx = 0;
    localNodeIdx leftChildNodeIdx = 1;
    localNodeIdx rightChildNodeIdx = 2;

    public:
    NodeMap(){};
    localNodeIdx getCurrentIdx() const { return currentNodeIdx; }
    localNodeIdx getParentIdx() const { return parentNodeIdx; }
    localNodeIdx getLeftChildIdx() const { return leftChildNodeIdx; }
    localNodeIdx getRightChildIdx() const { return rightChildNodeIdx; }

    void traverse(Direction direction){
        #pragma HLS INLINE
        auto freeIdx = parentNodeIdx;
        parentNodeIdx = currentNodeIdx;
        if(direction == LEFT){
            currentNodeIdx = leftChildNodeIdx;
            leftChildNodeIdx = freeIdx;
        }else{
            currentNodeIdx = rightChildNodeIdx;
            rightChildNodeIdx = freeIdx;
        }
    }
};

struct FetchRequest{
    localNodeIdx localIdx;
    int address;
};

enum ControllerState{
    START,
    START_WAIT,
    WAIT_FETCH,
    WAIT_PROCESS,
};

struct ProcessNodes{
    localNodeIdx parentNode;
    localNodeIdx currentNode;
};

struct NodeManager{
    NodeManager(){};
    Node_hbm createNewNode(){
        Node_hbm newNode;
        newNode.idx = getNextFreeIdx();
        return newNode;
    }

    private:
    int lastIdx = 0;
    
    int getNextFreeIdx(){
        if(lastIdx < MAX_NODES){
            return ++lastIdx;
        }
        return -1;
    }

};

void train(
    hls::stream<feature_vector> &featureStream,
    Node_hbm *nodePool,
    hls::stream<unit_interval> &rngStream,
    hls::stream<int> &rootNodeStream);

void process_node(NodeManager &nodeManager, NodeMap &m, feature_vector &feature, hls::stream<unit_interval> &rngStream, hls::stream<ProcessNodes> &process_nodes_stream, hls::stream<FetchRequest> &pre_fetch_stream, hls::stream<localNodeIdx> &save_address_stream, hls::stream<bool> &process_done_stream);
void prefetch_nodes(hls::stream<FetchRequest> &fetch_address_stream, hls::stream<Direction> &fetch_done_stream ,const Node_hbm *nodePool, Node_hbm *nodeBuffer);
void save_node(hls::stream<localNodeIdx> &save_address_stream, Node_hbm *nodePool,const Node_hbm *nodeBuffer);
void controller(const NodeMap &m, hls::stream<Direction> &fetch_done_stream, hls::stream<ProcessNodes> &process_nodes_stream, hls::stream<bool> &process_done_stream, hls::stream<int> &rootNodeStream, hls::stream<FetchRequest> &root_fetch_stream, hls::stream<feature_vector> &featureStream, feature_vector &feature);
void streamMerger(hls::stream<FetchRequest> &in1, hls::stream<FetchRequest> &in2, hls::stream<FetchRequest> &out);
void fetch_node_from_memory(int nodeAddress, localNodeIdx localIdx, hls::stream<Direction> &fetch_done_stream, const Node_hbm *nodePool, Node_hbm *nodeBuffer);


#endif