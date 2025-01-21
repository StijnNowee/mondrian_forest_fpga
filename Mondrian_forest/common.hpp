#ifndef COMMON_H_
#define COMMON_H_

#include <ap_fixed.h>
#include <cmath>
#include <hls_stream.h>
#include <limits>
#include <iostream>

#define FEATURE_COUNT_TOTAL 2
#define CLASS_COUNT 4

#define TREES_PER_BANK 5

//#define MAX_NODES 100 // Max nodes per bank
#define MAX_DEPTH 10
#define BANK_COUNT 1


//Page management
#define MAX_NODES_PER_PAGE 10
#define MAX_PAGES_PER_TREE 10

constexpr int log2_ceil(int n, int power = 0) {
    return (n <= (1 << power)) ? power : log2_ceil(n, power + 1);
}
constexpr int INTEGER_BITS = log2_ceil(FEATURE_COUNT_TOTAL);

typedef ap_ufixed<8, 0> unit_interval;
typedef ap_ufixed<INTEGER_BITS + 8, INTEGER_BITS> rate;

typedef ap_uint<8> label_vector;

typedef uint8_t localNodeIdx;

typedef unit_interval feature_vector[FEATURE_COUNT_TOTAL];

typedef ap_uint<1024> node_t;

struct input_vector {
    feature_vector feature;
    int label;

    input_vector() : feature{0, 0}, label(0) {}
};

struct ChildNode{
    bool isPage;
    int nodeIdx;
    ChildNode(bool isPage, int nodeIdx) : isPage(isPage), nodeIdx(nodeIdx){}
    ChildNode() : isPage(false), nodeIdx(0) {}
};

struct FetchRequest{
    input_vector input;
    int pageIdx;
    int treeID;
    bool done = false;
};


struct alignas(128) Node_hbm{ //__attribute__((packed)) 
    int idx;
    int parentIdx;
    bool leaf;
    bool valid;
    uint8_t feature;
    unit_interval threshold;
    float splittime;
    float parentSplitTime;
    feature_vector lowerBound;
    feature_vector upperBound;  
    unit_interval classDistribution[CLASS_COUNT];
    ChildNode leftChild;
    ChildNode rightChild;

    Node_hbm() : idx(0), parentIdx(0), leaf(false), valid(false), feature(0), threshold(0), splittime(0), parentSplitTime(0), lowerBound{0}, upperBound{0}, classDistribution{0}, leftChild(), rightChild(){}
};

union node_converter{
    Node_hbm node;
    node_t raw;
    node_converter() : node() {}
};

typedef node_t Page[MAX_NODES_PER_PAGE];
typedef node_t IPage[MAX_NODES_PER_PAGE + 1];

#endif