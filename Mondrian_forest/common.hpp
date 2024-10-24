#ifndef COMMON_H_
#define COMMON_H_

#include <ap_int.h>
#include <ap_fixed.h>
#include <cstdint>
#include <hls_stream.h>
#include <limits>

#define FEATURE_COUNT_TOTAL 44
#define CLASS_COUNT 7

#define FIXED_POINT_WIDTH 16
#define FIXED_POINT_INTEGER_BITS 4
#define TREE_COUNT 10
#define MAX_NODES 9 // Define a sufficiently large number for nodes

typedef ap_fixed<FIXED_POINT_WIDTH, FIXED_POINT_INTEGER_BITS> fixed_point;

typedef ap_uint<8> label_vector;

struct input_vector {
    fixed_point data[FEATURE_COUNT_TOTAL];
    fixed_point label;
};

struct feature_vector {
    fixed_point data[FEATURE_COUNT_TOTAL];
};



struct Node_hbm{
    int nodeIndex;
    bool leaf;
    uint8_t feature; //Can be much smaller, but at the cost of runtime computations. Don't know what is better
    fixed_point threshold;
    fixed_point lowerBound[FEATURE_COUNT_TOTAL];
    fixed_point upperBound[FEATURE_COUNT_TOTAL];
    fixed_point splittime;
    fixed_point classDistribution[CLASS_COUNT] = {};
    int leftChild;
    int rightChild;
    int parent;

    Node_hbm() : feature(-1), threshold(0.0), leftChild(0), rightChild(0), splittime(0.0), leaf(false){}
    Node_hbm(bool leaf, uint8_t feature = -1, fixed_point threshold = 0.0, int leftChild = -1, int rightChild = -1, fixed_point splittime = 0.0)
    : leaf(leaf), feature(feature), threshold(threshold), leftChild(leftChild), rightChild(rightChild), splittime(splittime) {}
};

struct Tree{
    int root;
    int currentNode;

    Tree(int root = -1) : root(root), currentNode(root) {}
};

#endif