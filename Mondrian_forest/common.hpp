#ifndef COMMON_H_
#define COMMON_H_

#include <ap_fixed.h>
#include <cmath>
#include <cstdint>
#include <hls_stream.h>
#include <limits>
#include <iostream>

constexpr int FEATURE_COUNT_TOTAL = 5;
constexpr int UNDEFINED_DIMENSION = FEATURE_COUNT_TOTAL + 1;
constexpr int CLASS_COUNT = 4;


constexpr int TREES_PER_BANK = 5;
constexpr int UPDATE_FEQUENCY = 10*TREES_PER_BANK; //In number of updates required

//#define MAX_NODES 100 // Max nodes per bank

constexpr int BANK_COUNT = 1;


//Page management
constexpr int MAX_NODES_PER_PAGE = 10;
constexpr int MAX_PAGES_PER_TREE = 20;

//Tree traversal
constexpr int MAX_DEPTH = MAX_NODES_PER_PAGE/2 + 1;
constexpr int MAX_ITERATION = MAX_NODES_PER_PAGE*2 -1;
constexpr int PAGE_SPLIT_TARGET = MAX_NODES_PER_PAGE/2;

constexpr int MAX_LIFETIME = 60000;

constexpr int log2_ceil(int n, int power = 0) {
    return (n <= (1 << power)) ? power : log2_ceil(n, power + 1);
}
constexpr int INTEGER_BITS = log2_ceil(FEATURE_COUNT_TOTAL + 1);
constexpr int CLASS_BITS = log2_ceil(CLASS_COUNT);

typedef ap_ufixed<8, 0> unit_interval;
typedef ap_ufixed<INTEGER_BITS + 8, INTEGER_BITS> rate_t;

typedef unit_interval feature_vector[FEATURE_COUNT_TOTAL];
typedef ap_ufixed<9, 1> classDistribution_t[CLASS_COUNT];

typedef ap_uint<1024> node_t;
typedef ap_ufixed<24,16> splitT_t;
typedef ap_uint<FEATURE_COUNT_TOTAL*8 + CLASS_BITS + 8> input_t;

constexpr int NODE_IDX_BITS = log2_ceil(MAX_NODES_PER_PAGE);

typedef ap_uint<NODE_IDX_BITS> nodeIdx_t;


struct __attribute__((packed)) input_vector {
    feature_vector feature;
    ap_uint<CLASS_BITS> label;
    bool inferenceSample;

    input_vector() : feature{0}, label(0), inferenceSample(false){}
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
    int freePageIdx;
    bool extraPage = false;
    bool needNewPage = false;
    bool updateSmlBank = false;
    bool shutdown = false;
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
    int labelCount;
    classDistribution_t classDistribution;
    ChildNode leftChild;
    ChildNode rightChild;

    Node_hbm() : idx(0), leaf(false), valid(false), feature(0), threshold(0), splittime(0), parentSplitTime(0), lowerBound{0}, upperBound{0}, labelCount(0), classDistribution{0}, leftChild(), rightChild(){}
    Node_hbm(ap_uint<8> feature, splitT_t splittime, splitT_t parentSplitTime, unit_interval threshold, bool leaf, int idx) : valid(true), feature(feature), splittime(splittime), parentSplitTime(parentSplitTime), threshold(threshold), leaf(leaf), idx(idx), lowerBound{0}, upperBound{0}, labelCount(0), classDistribution{0}, leftChild(), rightChild(){}
};

struct Node_sml{
    int leftChild;
    int rightChild;
    bool leaf;
    ap_uint<8> feature;
    unit_interval threshold;
    unit_interval upperBound;
    unit_interval lowerBound;
    classDistribution_t classDistribution;
     //IMPLEMENT CORRECT TRANSLATION FROM ap_ufixed<9,1> to ap_ufixed<8,0>
    
};

struct Result{
    //classDistribution_t distribution;
    ap_uint<CLASS_BITS> resultClass = 0;
    ap_ufixed<9,1> confidence = 0;
};

struct ClassDistribution{
    classDistribution_t distribution;
};

struct InputSizes{
    int total;
    int training;
    int inference;
};

typedef node_t Page[MAX_NODES_PER_PAGE];
typedef node_t IPage[MAX_NODES_PER_PAGE + 1];
typedef Node_sml tree_t[MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE];
typedef tree_t trees_t[TREES_PER_BANK];

#endif