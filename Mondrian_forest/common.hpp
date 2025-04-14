#ifndef COMMON_H_
#define COMMON_H_

#include <ap_fixed.h>
#include <ap_int.h>
#include <iostream>


constexpr int FEATURE_COUNT_TOTAL = 2; //54
constexpr int UNDEFINED_DIMENSION = FEATURE_COUNT_TOTAL + 1;
constexpr int CLASS_COUNT = 3; //7


constexpr int TREES_PER_BANK = 12;
constexpr size_t BLOCK_SIZE = 500;

constexpr int BANK_COUNT = 16;
constexpr int TRAIN_TRAVERSAL_BLOCKS = 3;
constexpr int INF_TRAVERSAL_BLOCKS = 3;


//Page management
constexpr int MAX_NODES_PER_PAGE = 31; //31
constexpr int MAX_PAGES_PER_TREE = 1000; //1000

//Tree traversal
constexpr int MAX_DEPTH = MAX_NODES_PER_PAGE/2 + 1;
constexpr int MAX_TREE_MAP_ITER = MAX_NODES_PER_PAGE*2 -1;
constexpr int PAGE_SPLIT_TARGET = MAX_NODES_PER_PAGE/2;
constexpr int MAX_EXPECTED_LEAFS = MAX_NODES_PER_PAGE*MAX_PAGES_PER_TREE/2;

constexpr int MAX_LIFETIME = 1000;
constexpr float ALPHA = 0.5;
constexpr float BETA = CLASS_COUNT/2.0;


constexpr int log2_ceil(int n, int power = 0) {
    return (n <= (1 << power)) ? power : log2_ceil(n, power + 1);
}
constexpr int INTEGER_BITS = log2_ceil(FEATURE_COUNT_TOTAL + 1);
constexpr int CLASS_BITS = log2_ceil(CLASS_COUNT);

typedef ap_ufixed<8, 0> unit_interval;
typedef ap_ufixed<INTEGER_BITS + 8, INTEGER_BITS> rate_t;
typedef ap_uint<8> ap_byte_t;
typedef unit_interval feature_vector[FEATURE_COUNT_TOTAL];
typedef ap_ufixed<8, 0> weight_t[CLASS_COUNT];

typedef ap_uint<1024> node_t;
typedef ap_ufixed<16,10> splitT_t;
typedef ap_uint<FEATURE_COUNT_TOTAL*8 + CLASS_BITS + 8> input_t;

constexpr int NODE_IDX_BITS = log2_ceil(MAX_NODES_PER_PAGE);

typedef ap_uint<NODE_IDX_BITS> nodeIdx_t;
typedef ap_uint<1> ap_bool_t;

constexpr float H = 1.0/CLASS_COUNT;

class PageProperties;
class IPageProperties;
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

    const bool isPage() const {return child[0];};
    const int id() const {return child.range(15, 1);};

    ChildNode(const bool &isPagev, const int &idv) {isPage(isPagev), id(idv);};
    ChildNode() : child(0) {}
};
struct ClassDistribution{
    weight_t dis = {0};
};

struct Feedback{
    input_vector input;
    int treeID;
    int pageIdx;
    bool extraPage = false;
    bool needNewPage = false;
    int freePageIdx;
    Feedback(){};
    Feedback(const PageProperties &p, const bool &extraPage);

};

struct IFeedback : Feedback{
    ClassDistribution s;
    bool isOutput = false;
    IFeedback(){};
    IFeedback(const IPageProperties &p);
};

struct FetchRequest{
    input_vector input;
    int pageIdx;
    int treeID;
    int freePageIdx;
    FetchRequest(){};
    FetchRequest(const Feedback &feedback) : pageIdx(feedback.pageIdx), treeID(feedback.treeID), input(feedback.input), freePageIdx(feedback.freePageIdx){};
    FetchRequest(const input_vector &input, const int &pageIdx, const int &treeID, const int &freePageIdx) : input(input), pageIdx(pageIdx), treeID(treeID), freePageIdx(freePageIdx){};
};

struct IFetchRequest : FetchRequest{
    ClassDistribution s = {0};
    bool isOutput = false;
    IFetchRequest(){};
    IFetchRequest(const IFeedback &feedback) : FetchRequest(feedback), isOutput(feedback.isOutput) {
        for(int c = 0; c < CLASS_COUNT; c++){
            #pragma HLS PIPELINE II=1
            s.dis[c] = feedback.s.dis[c];
        }
    };
    IFetchRequest(const input_vector &input, const int &pageIdx, const int &treeID) : FetchRequest(input, pageIdx, treeID, 0){};
};

 enum Directions{
     LEFT,
     RIGHT
 };

struct __attribute__((packed)) alignas(128) Node_hbm{
    ap_byte_t combi;
    unit_interval threshold;
    ap_byte_t feature;
    splitT_t splittime;
    splitT_t parentSplitTime;
    feature_vector lowerBound;
    feature_vector upperBound;
    weight_t weight;
    ap_byte_t counts[CLASS_COUNT];
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
    bool setTab(const Directions &dir, const int &classIdx){
        if(!counts[classIdx][dir]){
            counts[classIdx][dir] = true;
            return true;
        }
        return false;
    }

    const bool leaf() const{  
        #pragma HLS inline 
        return combi[0];};
    const int idx()const{  
        #pragma HLS inline 
        return combi.range(NODE_IDX_BITS, 1);};
    const bool valid()const{  
        #pragma HLS inline 
        return combi[NODE_IDX_BITS + 1];};
    const bool getTab(const Directions &dir, const int &classIdx){
        return counts[classIdx][dir];
    }
    const ap_byte_t getTotalTabs(const int &classIdx){
        return counts[classIdx][0] + counts[classIdx][1];
    }

    Node_hbm() : combi(0), feature(0), threshold(0), splittime(0), parentSplitTime(0), lowerBound{0}, upperBound{0}, counts{0}, leftChild(), rightChild(){}
    Node_hbm(ap_uint<8> feature, splitT_t splittime, splitT_t parentSplitTime, unit_interval threshold, bool leafv, int idxv) : feature(feature), splittime(splittime), parentSplitTime(parentSplitTime), threshold(threshold), leftChild(), rightChild(), counts{0}{ valid(true); idx(idxv); leaf(leafv);}
};

struct Result{
    //classDistribution_t distribution;
    ap_uint<CLASS_BITS> resultClass = 0;
    unit_interval confidence = 0;
};

struct InputSizes{
    int total = 0;
    int seperate[2] = {0};
};



typedef node_t Page[MAX_NODES_PER_PAGE];
typedef Page PageBank[MAX_PAGES_PER_TREE*TREES_PER_BANK];
typedef node_t IPage[MAX_NODES_PER_PAGE + 1];

struct SplitProperties{
    bool enabled;
    int nodeIdx;
    int dimension;
    int parentIdx;
    splitT_t newSplitTime;
    unit_interval rngVal;
    bool sampleSplit;

    SplitProperties() : enabled(false), nodeIdx(0), dimension(0), parentIdx(0), newSplitTime(0), rngVal(0), sampleSplit(false) {}
    SplitProperties(bool enabled, int nodeIdx, int dimension, int parentIdx, splitT_t newSplitTime, unit_interval rngVal, bool sampleSplit) : enabled(enabled), nodeIdx(nodeIdx), dimension(dimension), parentIdx(parentIdx), newSplitTime(newSplitTime), rngVal(rngVal), sampleSplit(sampleSplit){}
};

struct PageProperties{
    input_vector input;
    bool needNewPage = false;
    bool extraPage = false;
    int pageIdx;
    int nextPageIdx;
    int treeID;
    int freeNodesIdx[2] = {-1, -1};
    int freePageIdx;
    bool shouldSave = false;
    bool splitPage = false;
    SplitProperties split;
    

    PageProperties(){};
    PageProperties(FetchRequest &request) : input(request.input), pageIdx(request.pageIdx), treeID(request.treeID), freePageIdx(request.freePageIdx), shouldSave(true){};
    void setSplitProperties(int nodeIdx, int dimension, int parentIdx, splitT_t newSplitTime, unit_interval rngVal, bool sampleSplit) {
        split = SplitProperties(true, nodeIdx, dimension, parentIdx, newSplitTime, rngVal, sampleSplit);
    }
    
};

struct IPageProperties : PageProperties{
    ClassDistribution s;
    bool isOutput = false;
    IPageProperties(){};
    IPageProperties(IFetchRequest &request) : PageProperties(request), isOutput(request.isOutput){
        for(int c = 0; c < CLASS_COUNT; c++){
            #pragma HLS PIPELINE II=1
            s.dis[c] = request.s.dis[c];
        }
    }
};

enum SampleType{
    INF,
    TRAIN
};
#endif