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
    ap_uint<2> parentNodeIdx = 3;
    ap_uint<2> currentNodeIdx = 0;
    ap_uint<2> leftChildNodeIdx = 1;
    ap_uint<2> rightChildNodeIdx = 2;

    public:
    ap_uint<2> getCurrentNodeIdx() const { return currentNodeIdx; }
    ap_uint<2> getParentNodeIdx() const { return parentNodeIdx; }
    ap_uint<2> getLeftChildNodeIdx() const { return leftChildNodeIdx; }
    ap_uint<2> getRightChildNodeIdx() const { return rightChildNodeIdx; }

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

struct NodeManager{

    void fetch_root(int &address, const Node_hbm *nodePool);
    void prefetch_nodes(const Node_hbm *nodePool);
    void save_node(Node_hbm &node, Node_hbm *nodePool);
    void update_node(Node_hbm *nodePool);
    void prepare_next_nodes();

    Node_hbm& getParent(){return nodeBuffer[m.getParentNodeIdx()];};
    Node_hbm& getCurrent(){return nodeBuffer[m.getCurrentNodeIdx()];};
    Node_hbm& getLeftChild(){return nodeBuffer[m.getLeftChildNodeIdx()];};
    Node_hbm& getRightChild(){return nodeBuffer[m.getRightChildNodeIdx()];};

    void setNextDirection(Direction dir){nextDir = dir;};

    Node_hbm createNewNode(){
        Node_hbm newNode;
        newNode.idx = getNextFreeIdx();
        return newNode;
    }

    private:
    Node_hbm nodeBuffer[4];
    NodeMap m;
    Direction nextDir;

    int leftChildAddress, rightChildAddress;
    int lastIdx = 0;

    void fetch_node_from_memory(int nodeAddress, ap_uint<2> localIdx, const Node_hbm *nodePool);
    
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
    Tree &tree,
    Node_hbm *nodePool,
    hls::stream<unit_interval> &rngStream);

//void fetch_node_from_memory(int &nodeAddress, ap_uint<2> localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer);
//void prefetch_node(NodeMap &nodeMap, Node_hbm* nodeBuffer, int &leftChildAddress, int &rightChildAddress, Node_hbm *nodePool);
void process_node(NodeManager &nodeManager, feature_vector &feature, hls::stream<unit_interval> &rngStream, bool &done, Node_hbm *nodePool);
//void update_node(ap_uint<2> &localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer);
//void save_new_node(Node_hbm &newNode, Node_hbm *nodePool);

void parallel_prefetch_process(NodeManager &nodeManager, feature_vector &feature, hls::stream<unit_interval> &rngStream, bool &done, Node_hbm *nodePool);
//void prepare_next_nodes(Node_hbm *nodeBuffer, NodeMap &nodeMap, uint8_t &depth, int &leftChildAddress, int &rightChildAddress, feature_vector &feature);



#endif