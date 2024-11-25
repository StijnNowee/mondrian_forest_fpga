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
    localNodeIdx getCurrentNodeIdx() const { return currentNodeIdx; }
    localNodeIdx getParentNodeIdx() const { return parentNodeIdx; }
    localNodeIdx getLeftChildNodeIdx() const { return leftChildNodeIdx; }
    localNodeIdx getRightChildNodeIdx() const { return rightChildNodeIdx; }

    void traverse(Direction &direction){
        #pragma HLS INLINE
        auto freeIdx = this->parentNodeIdx;
        this->parentNodeIdx = this->currentNodeIdx;
        if(direction == LEFT){
            this->currentNodeIdx = this->leftChildNodeIdx;
            this->leftChildNodeIdx = freeIdx;
        }else{
            this->currentNodeIdx = this->rightChildNodeIdx;
            this->rightChildNodeIdx = freeIdx;
        }
    }
};

struct FetchRequest{
    localNodeIdx localIdx;
    int address;
};

enum ControllerState{
    START,
    WAIT_FETCH,
    WAIT_PROCESS,
};

struct ProcessNodes{
    localNodeIdx parentNode;
    localNodeIdx currentNode;
};

struct NodeManager{
    NodeManager(){
        #pragma HLS AGGREGATE variable=nodeBuffer compact=auto
        #pragma HLS ARRAY_PARTITION variable=nodeBuffer dim=1 type=complete
    }
    void fetch_root(int &address, hls::stream<Direction> &fetch_done_stream, const Node_hbm *nodePool);
    void prefetch_nodes(hls::stream<FetchRequest> &fetch_address_stream, hls::stream<Direction> &fetch_done_stream, const Node_hbm *nodePool);
    void save_node(hls::stream<localNodeIdx> &save_address_stream, Node_hbm *nodePool);
    void update_node(Node_hbm *nodePool);
    void prepare_next_nodes(hls::stream<FetchRequest> &fetch_address_stream);
    void controller(hls::stream<Direction> &fetch_done_stream, hls::stream<ProcessNodes> &process_nodes_stream, hls::stream<bool> &process_done_stream, hls::stream<int> &rootNodeStream, hls::stream<FetchRequest> &root_fetch_stream);

    // Node_hbm& getParent(){return nodeBuffer[m.getParentNodeIdx()];};
    // Node_hbm& getCurrent(){return nodeBuffer[m.getCurrentNodeIdx()];};
    // Node_hbm& getLeftChild(){return nodeBuffer[m.getLeftChildNodeIdx()];};
    // Node_hbm& getRightChild(){return nodeBuffer[m.getRightChildNodeIdx()];};

    // localNodeIdx getCurrentIdx(){return m.getCurrentNodeIdx();};
    //void setNextDirection(Direction dir){nextDir = dir;};

    void traverse(Direction dir){m.traverse(dir);};
    Node_hbm& getNode(localNodeIdx localNodeIdx){return nodeBuffer[localNodeIdx];};

    localNodeIdx getLeftChildIdx(){return m.getLeftChildNodeIdx();};
    localNodeIdx getRightChildIdx(){return m.getRightChildNodeIdx();};

    Node_hbm createNewNode(){
        Node_hbm newNode;
        newNode.idx = getNextFreeIdx();
        return newNode;
    }

    private:
    Node_hbm nodeBuffer[4];
    NodeMap m;
    ControllerState state = START;
    //Direction nextDir;

    int lastIdx = 0;

    void fetch_node_from_memory(int nodeAddress, localNodeIdx localIdx, hls::stream<Direction> &fetch_done_stream, const Node_hbm *nodePool);
    
    int getNextFreeIdx(){
        if(lastIdx < MAX_NODES){
            return ++lastIdx;
        }
        return -1;
    }

};

void train(
    feature_vector &feature,
    label_vector   &label,
    Node_hbm *nodePool,
    hls::stream<unit_interval> &rngStream,
    hls::stream<int> &rootNodeStream);

//void fetch_node_from_memory(int &nodeAddress, localNodeIdx localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer);
//void prefetch_node(NodeMap &nodeMap, Node_hbm* nodeBuffer, int &leftChildAddress, int &rightChildAddress, Node_hbm *nodePool);
void process_node(NodeManager &nodeManager, feature_vector &feature, hls::stream<unit_interval> &rngStreaml, hls::stream<ProcessNodes> &process_nodes_stream, hls::stream<FetchRequest> &pre_fetch_stream, hls::stream<localNodeIdx> &save_address_stream, hls::stream<bool> &process_done_stream);
//void update_node(localNodeIdx &localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer);
//void save_new_node(Node_hbm &newNode, Node_hbm *nodePool);

void parallel_prefetch_process(NodeManager &nodeManager, feature_vector &feature, hls::stream<unit_interval> &rngStream, bool &done, Node_hbm *nodePool);
//void prepare_next_nodes(Node_hbm *nodeBuffer, NodeMap &nodeMap, uint8_t &depth, int &leftChildAddress, int &rightChildAddress, feature_vector &feature);
void streamMerger(hls::stream<FetchRequest> &in1, hls::stream<FetchRequest> &in2, hls::stream<FetchRequest> &out);


#endif