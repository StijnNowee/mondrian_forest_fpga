#ifndef COMMON_H_
#define COMMON_H_

#include <ap_fixed.h>
#include <ap_int.h>
#include <iostream>


constexpr int FEATURE_COUNT_TOTAL = 54;
constexpr int UNDEFINED_DIMENSION = FEATURE_COUNT_TOTAL + 1;
constexpr int CLASS_COUNT = 7;


constexpr int TREES_PER_BANK = 5;
constexpr int UPDATE_FEQUENCY = 500*TREES_PER_BANK; //In number of updates required

//#define MAX_NODES 100 // Max nodes per bank

constexpr int BANK_COUNT = 1;
constexpr int TRAVERSAL_BLOCKS = 3;


//Page management
constexpr int MAX_NODES_PER_PAGE = 31;
constexpr int MAX_PAGES_PER_TREE = 1000;

//Tree traversal
constexpr int MAX_DEPTH = MAX_NODES_PER_PAGE/2 + 1;
constexpr int MAX_ITERATION = MAX_NODES_PER_PAGE*2 -1;
constexpr int PAGE_SPLIT_TARGET = MAX_NODES_PER_PAGE/2;

constexpr int MAX_LIFETIME = 4000;

constexpr int log2_ceil(int n, int power = 0) {
    return (n <= (1 << power)) ? power : log2_ceil(n, power + 1);
}
constexpr int INTEGER_BITS = log2_ceil(FEATURE_COUNT_TOTAL + 1);
constexpr int CLASS_BITS = log2_ceil(CLASS_COUNT);
constexpr int SML_LEAF_BYTES = 2 + CLASS_COUNT;
constexpr int SML_NODE_BYTES = 6;
constexpr int SML_COMBINED_BITS = (SML_LEAF_BYTES < SML_NODE_BYTES) ? SML_NODE_BYTES*8 : SML_LEAF_BYTES*8;

typedef ap_uint<SML_COMBINED_BITS> sml_t;

typedef ap_ufixed<8, 0> unit_interval;
typedef ap_ufixed<INTEGER_BITS + 8, INTEGER_BITS> rate_t;

typedef unit_interval feature_vector[FEATURE_COUNT_TOTAL];
typedef ap_ufixed<8, 0, AP_TRN, AP_SAT> classDistribution_t[CLASS_COUNT];

typedef ap_uint<1024> node_t;
typedef ap_ufixed<16,12> splitT_t;
typedef ap_uint<FEATURE_COUNT_TOTAL*8 + CLASS_BITS + 8> input_t;

constexpr int NODE_IDX_BITS = log2_ceil(MAX_NODES_PER_PAGE);

typedef ap_uint<NODE_IDX_BITS> nodeIdx_t;
typedef ap_uint<1> ap_bool_t;
typedef ap_uint<8> ap_byte_t;

struct __attribute__((packed)) input_vector {
    feature_vector feature;
    ap_uint<CLASS_BITS> label;
    bool inferenceSample;

    input_vector() : feature{0}, label(0), inferenceSample(false){}
};

struct ChildNode{
    ap_uint<16> child;

    void isPage(const bool &isPage){child[0] = isPage;};
    void id(const int &id){child.range(15, 1) = id;};

    const bool isPage(){return child[0];};
    const int id(){return child.range(15, 1);};

    ChildNode(const bool &isPagev, const int &idv) {isPage(isPagev), id(idv);};
    ChildNode() : child(0) {}
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


struct __attribute__((packed)) alignas(128) Node_hbm{
    ap_byte_t combi;
    unit_interval threshold;
    ap_byte_t feature;
    splitT_t splittime;
    splitT_t parentSplitTime;
    feature_vector lowerBound;
    feature_vector upperBound;  
    short int labelCount;
    classDistribution_t classDistribution;
    ChildNode leftChild;
    ChildNode rightChild;

    void leaf(const bool &leaf){ 
        #pragma HLS inline 
        combi[0] = leaf;};
    void idx(const int &idx){  
        #pragma HLS inline 
        combi.range(NODE_IDX_BITS, 1) = idx;}
    void valid(const bool &valid){  
        #pragma HLS inline 
        combi[NODE_IDX_BITS + 1] = valid;};

    const bool leaf(){  
        #pragma HLS inline 
        return combi[0];};
    const int idx(){  
        #pragma HLS inline 
        return combi.range(NODE_IDX_BITS, 1);};
    const bool valid(){  
        #pragma HLS inline 
        return combi[NODE_IDX_BITS + 1];};

    Node_hbm() : combi(0), feature(0), threshold(0), splittime(0), parentSplitTime(0), lowerBound{0}, upperBound{0}, labelCount(0), classDistribution{0}, leftChild(), rightChild(){}
    Node_hbm(ap_uint<8> feature, splitT_t splittime, splitT_t parentSplitTime, unit_interval threshold, bool leafv, int idxv) : feature(feature), splittime(splittime), parentSplitTime(parentSplitTime), threshold(threshold), lowerBound{0}, upperBound{0}, labelCount(0), classDistribution{0}, leftChild(), rightChild(){ valid(true); idx(idxv); leaf(leafv);}
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
};

struct Result{
    //classDistribution_t distribution;
    ap_uint<CLASS_BITS> resultClass = 0;
    unit_interval confidence = 0;
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
typedef Page PageBank[MAX_PAGES_PER_TREE*TREES_PER_BANK];
typedef node_t IPage[MAX_NODES_PER_PAGE + 1];
typedef Node_sml tree_t[MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE];
typedef tree_t trees_t[TREES_PER_BANK];

#endif