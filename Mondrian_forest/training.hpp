#ifndef TRAINING_HPP
#define TRAINING_HPP

#include "common.hpp"

extern "C" {

    struct NodeMap {
        ap_uint<2> parentNodeIdx = 3;
        ap_uint<2> currentNodeIdx = 0;
        ap_uint<2> leftChildNodeIdx = 1;
        ap_uint<2> rightChildNodeIdx = 2;
    };

    enum NodeType{
        PARENT,
        CURRENT,
        LEFTCHILD,
        RIGHTCHILD
    };

    enum State{
        START,
        PROCESS,
        WAITFORFETCH,
    };


    void train(
        feature_vector &feature,
        label_vector   &label,
        Tree &tree,
        Node_hbm *nodePool);

    //void fetch_node(hls::stream<int> &nodeRequestStream, hls::stream<ap_uint<2>> &nodeStoredStream, Node_hbm *nodePool, Node_hbm *nodeBuffer);
    void fetch_node_from_memory(int &nodeAddress, ap_uint<2> &localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer);
    void prefetch_node(NodeMap &nodeMap, Node_hbm *nodeBuffer, Node_hbm *nodePool, hls::stream<ap_uint<1>> &fetch_done);

    //void process_node(hls::stream<ap_uint<3>> &nodeStoredStream, hls::stream<ap_uint<2>> &nodeSaveStream, hls::stream<int> &nodeRequestStream, Node_hbm *nodeBuffer);
    void save_node(ap_uint<2> &localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer);
    void wait_for_fetch(hls::stream<ap_uint<1>> &left_fetch_done, hls::stream<ap_uint<1>> &right_fetch_done, hls::stream<ap_uint<1>> &fetch_done);

    void nextNode(ap_uint<2> &nextNodeIdx, NodeMap &nodeMap);
    void process_node(Node_hbm &currentNode);

    // void createMondrianTree(Tree *tree, feature_vector &feature, Node_hbm *nodePool);
    // void ExtendMondrianBlock(Tree *tree, int node, feature_vector &feature, hls::stream<fixed_point> &rng, Node_hbm *nodePool);


    // void splitNode(Node_hbm *nodePool, Node_hbm &currentNode, fixed_point &potentialSplitTime, fixed_point &totalBounds, hls::stream<fixed_point> &rng, fixed_point *weights, fixed_point *eu, fixed_point *el, feature_vector &feature, int &freeNodeID);
    // void updateAndTraverse(Tree *localTree, Node_hbm &localNode, feature_vector *feature, fixed_point *eu, fixed_point *el);

}



#endif