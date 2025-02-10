#ifndef COMMON_H_
#define COMMON_H_

#include <ap_fixed.h>
#include <cmath>
#include <cstdint>
#include <hls_stream.h>
#include <limits>
#include <iostream>

constexpr int FEATURE_COUNT_TOTAL = 2;
constexpr int UNDEFINED_DIMENSION = FEATURE_COUNT_TOTAL + 1;
constexpr int CLASS_COUNT = 4;

constexpr int TREES_PER_BANK = 1;

//#define MAX_NODES 100 // Max nodes per bank

constexpr int BANK_COUNT = 1;


//Page management
constexpr int MAX_NODES_PER_PAGE = 31;
constexpr int MAX_PAGES_PER_TREE = 20;

//Tree traversal
constexpr int MAX_DEPTH = MAX_NODES_PER_PAGE/2 + 1;
constexpr int MAX_ITERATION = MAX_NODES_PER_PAGE*2 -1;
constexpr int PAGE_SPLIT_TARGET = MAX_NODES_PER_PAGE/2;

constexpr int MAX_LIFETIME = 60000;

constexpr int log2_ceil(int n, int power = 0) {
    return (n <= (1 << power)) ? power : log2_ceil(n, power + 1);
}
constexpr int INTEGER_BITS = log2_ceil(FEATURE_COUNT_TOTAL);

typedef ap_ufixed<8, 0> unit_interval;
typedef ap_ufixed<INTEGER_BITS + 8, INTEGER_BITS> rate_t;

typedef unit_interval feature_vector[FEATURE_COUNT_TOTAL];

typedef ap_uint<1024> node_t;
typedef ap_ufixed<24,16> splitT_t;

struct __attribute__((packed)) input_vector {
    feature_vector feature;
    int label;

    input_vector() : feature{0, 0}, label(0) {}
};

struct ChildNode{
    bool isPage;
    int id;
    ChildNode(bool isPage, int id) : isPage(isPage), id(id){}
    ChildNode() : isPage(false), id(0) {}
};

struct FetchRequest{
    input_vector input;
    int pageIdx;
    int treeID;
    bool done = false;
    bool needNewPage = false;
};


struct alignas(128) Node_hbm{
    int idx;
    bool leaf;
    bool valid;
    ap_uint<8> feature;
    unit_interval threshold;
    splitT_t splittime;
    splitT_t parentSplitTime;
    feature_vector lowerBound;
    feature_vector upperBound;  
    unit_interval classDistribution[CLASS_COUNT];
    ChildNode leftChild;
    ChildNode rightChild;

    Node_hbm() : idx(0), leaf(false), valid(false), feature(0), threshold(0), splittime(0), parentSplitTime(0), lowerBound{0}, upperBound{0}, classDistribution{0}, leftChild(), rightChild(){}
    Node_hbm(ap_uint<8> feature, splitT_t splittime, splitT_t parentSplitTime, unit_interval threshold, bool leaf, int idx) : valid(true), feature(feature), splittime(splittime), parentSplitTime(parentSplitTime), threshold(threshold), leaf(leaf), idx(idx), lowerBound{0}, upperBound{0}, classDistribution{0}, leftChild(), rightChild(){}
};

// union node_converter{
//     Node_hbm node;
//     node_t raw;
//     node_converter() : node() {}
//     node_converter(node_t raw) : raw(raw) {}
//     node_converter(ap_uint<8> feature, splitT_t splittime, splitT_t parentSplitTime, unit_interval threshold, bool leaf, int idx = 0) : node(feature, splittime, parentSplitTime, threshold, leaf, idx) {}
// };

typedef node_t Page[MAX_NODES_PER_PAGE];
typedef node_t IPage[MAX_NODES_PER_PAGE + 1];

#endif