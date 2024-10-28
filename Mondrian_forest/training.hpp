#ifndef TRAINING_HPP
#define TRAINING_HPP

#include "common.hpp"

extern "C" {
    static int nodeIndex = 0;

    void train(
        hls::stream<feature_vector> &featureStream,
        hls::stream<label_vector>   &labelStream,
        hls::stream<fixed_point> &rng,
        hls::stream<Tree> &treeInfoStream,
        hls::stream<Tree> &treeOutputStream,
        Node_hbm *nodePool,
        int &freeNodeID);

    void createMondrianTree(Tree *tree, feature_vector &feature, Node_hbm *nodePool, int &freeNodeID);
    void ExtendMondrianBlock(Tree *tree, int node, feature_vector &feature, hls::stream<fixed_point> &rng, Node_hbm *nodePool, int &freeNodeID);


    void splitNode(Node_hbm *nodePool, Node_hbm &currentNode, fixed_point &potentialSplitTime, fixed_point &totalBounds, hls::stream<fixed_point> &rng, fixed_point *weights, fixed_point *eu, fixed_point *el, feature_vector &feature, int &freeNodeID);
    void updateAndTraverse(Tree *localTree, Node_hbm &localNode, feature_vector *feature, fixed_point *eu, fixed_point *el);
}



#endif