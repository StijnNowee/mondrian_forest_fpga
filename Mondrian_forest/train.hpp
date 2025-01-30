#ifndef TRAIN_HPP
#define TRAIN_HPP
#include "common.hpp"
#include "hls_streamofblocks.h"

struct SplitProperties{
    bool enabled;
    int nodeIdx;
    int dimension;
    int parentIdx;
    float newSplitTime;

    SplitProperties() : enabled(false), nodeIdx(0), dimension(0), parentIdx(0), newSplitTime(0) {}
    SplitProperties(bool enabled, int nodeIdx, int dimension, int parentIdx, float newSplitTime) : enabled(enabled), nodeIdx(nodeIdx), dimension(dimension), parentIdx(parentIdx), newSplitTime(newSplitTime) {}
};

struct alignas(128) PageProperties{
    input_vector input;
    bool dontIterate;
    int pageIdx;
    int nextPageIdx;
    int treeID;
    int freeNodesIdx[2];
    SplitProperties split;
    

    PageProperties() : input(), pageIdx(0), nextPageIdx(0), freeNodesIdx{-1, -1}, treeID(0), split(), dontIterate(false) {}
    PageProperties(input_vector input, int pageIdx, int treeID) : input(input), pageIdx(pageIdx), treeID(treeID), freeNodesIdx{-1, -1}, split(), nextPageIdx(0), dontIterate(false) {}
    void setSplitProperties(int nodeIdx, int dimension, int parentIdx, float newSplitTime) {
        split = SplitProperties(true, nodeIdx, dimension, parentIdx, newSplitTime);
    }
};

node_t convertPropertiesToRaw(const PageProperties &p);

PageProperties convertRawToProperties(const node_t &raw);

// union p_converter{
//     PageProperties p;
//     node_t raw;

//     p_converter() : p(){}
//     p_converter(PageProperties &p) : p(p) {} 
//     p_converter(node_t raw) : raw(raw) {}
//     p_converter(input_vector input, int pageIdx, int treeID) : p(input, pageIdx, treeID) {}
    
//    // p_converter(input_vector input, int pageIdx, int treeID, bool feedback) : p(input, pageIdx, treeID, feedback) {}
// };

enum TreeStatus{
    PROCESSING,
    IDLE
};

struct PageSplit{
    int bestSplitValue = MAX_NODES_PER_PAGE;
    int bestSplitLocation = 0;
    int nrOfBranchedNodes = 0;
};

void feature_distributor(hls::stream<input_vector> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], int size);
void pre_fetcher(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, const int loopCount, hls::stream<bool> treeDoneStream[4]);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval,100> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut, const int loopCount, hls::stream<bool> &treeDoneStream);
void page_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream_of_blocks<IPage> &pageOut, const int loopCount, hls::stream<bool> &treeDoneStream);
void node_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval,100> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut, const int loopCount, hls::stream<bool> &treeDoneStream);
void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool, const int loopCount, hls::stream<bool> &treeDoneStream);



#endif