#ifndef TRAINING_HPP
#define TRAINING_HPP

#include "common.hpp"

extern "C" {

    void train(
        feature_vector &feature,
        label_vector   &label,
        Tree &tree,
        Node_hbm *nodePool);

    void fetch_node(hls::stream<int> &nodeRequestStream, hls::stream<Node_hbm> &nodeFetchStream, Node_hbm *nodePool);
    void process_node(hls::stream<Node_hbm> &nodeFetchStream, hls::stream<Node_hbm> &nodeSaveStream, hls::stream<int> &nodeRequestStream);
    void save_node(hls::stream<Node_hbm> &nodeSaveStream, Node_hbm *nodePool);

    // void createMondrianTree(Tree *tree, feature_vector &feature, Node_hbm *nodePool);
    // void ExtendMondrianBlock(Tree *tree, int node, feature_vector &feature, hls::stream<fixed_point> &rng, Node_hbm *nodePool);


    // void splitNode(Node_hbm *nodePool, Node_hbm &currentNode, fixed_point &potentialSplitTime, fixed_point &totalBounds, hls::stream<fixed_point> &rng, fixed_point *weights, fixed_point *eu, fixed_point *el, feature_vector &feature, int &freeNodeID);
    // void updateAndTraverse(Tree *localTree, Node_hbm &localNode, feature_vector *feature, fixed_point *eu, fixed_point *el);
}



#endif