#ifndef TRAINING_HPP
#define TRAINING_HPP

#include "common.hpp"

extern "C" {

    void train(
        feature_vector &feature,
        label_vector   &label,
        Tree &tree,
        Node_hbm *nodePool);

    void fetch_node(hls::stream<int> &nodeRequestStream, hls::stream<ap_uint<2>> &nodeStoredStream, Node_hbm *nodePool, Node_hbm *nodeBuffer);
    void fetch_node_from_memory(int nodeAddress, ap_uint<2> localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer, hls::stream<ap_uint<2>> &nodeStoredStream, bool *available);
    void prefetch_node(ap_uint<2> localNodeIdx, Node_hbm *nodeBuffer, bool *available, Node_hbm *nodePool, hls::stream<ap_uint<2>> &nodeStoredStream);

    void process_node(hls::stream<ap_uint<2>> &nodeStoredStream, hls::stream<ap_uint<2>> &nodeSaveStream, hls::stream<int> &nodeRequestStream, Node_hbm *nodeBuffer);
    void save_node(ap_uint<2> localNodeAddress, Node_hbm *nodePool, Node_hbm *nodeBuffer, bool *available);

    // void createMondrianTree(Tree *tree, feature_vector &feature, Node_hbm *nodePool);
    // void ExtendMondrianBlock(Tree *tree, int node, feature_vector &feature, hls::stream<fixed_point> &rng, Node_hbm *nodePool);


    // void splitNode(Node_hbm *nodePool, Node_hbm &currentNode, fixed_point &potentialSplitTime, fixed_point &totalBounds, hls::stream<fixed_point> &rng, fixed_point *weights, fixed_point *eu, fixed_point *el, feature_vector &feature, int &freeNodeID);
    // void updateAndTraverse(Tree *localTree, Node_hbm &localNode, feature_vector *feature, fixed_point *eu, fixed_point *el);
}



#endif