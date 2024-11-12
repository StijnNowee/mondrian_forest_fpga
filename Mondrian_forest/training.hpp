#ifndef TRAINING_HPP
#define TRAINING_HPP

#include "common.hpp"
#include <cstdint>

extern "C" {
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

    void train(
        feature_vector &feature,
        label_vector   &label,
        Tree &tree,
        Node_hbm *nodePool);

    void fetch_node_from_memory(int &nodeAddress, ap_uint<2> localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer);
    void prefetch_node(NodeMap &nodeMap, Node_hbm* nodeBuffer, int &leftChildAddress, int &rightChildAddress, Node_hbm *nodePool);
    void process_node(Node_hbm &currentNode, Node_hbm &parentNode);
    void save_node(ap_uint<2> &localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer);
   
    void parallel_prefetch_process(NodeMap &m, Node_hbm *nodeBuffer, int &leftChildAddress, int &rightChildAddress, Node_hbm *nodePool);
    void prepare_next_nodes(Node_hbm *nodeBuffer, NodeMap &nodeMap, bool &done, uint8_t &depth, int &leftChildAddress, int &rightChildAddress);

}



#endif