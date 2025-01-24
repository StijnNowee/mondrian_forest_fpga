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
    int pageIdx;
    int nextPageIdx;
    int freeNodesIdx[2];
    bool feedback;
    int treeID;
    SplitProperties split;

    PageProperties() : input(), pageIdx(0), nextPageIdx(0), freeNodesIdx{-1, -1}, feedback(false), treeID(0), split() {}
    PageProperties(input_vector input, int pageIdx, int treeID, bool feedback) : input(input), pageIdx(pageIdx), treeID(treeID), feedback(feedback), freeNodesIdx{-1, -1}, split(), nextPageIdx(0) {}
    void setSplitProperties(int nodeIdx, int dimension, int parentIdx, float newSplitTime) {
        split = SplitProperties(true, nodeIdx, dimension, parentIdx, newSplitTime);
    }
};

union p_converter{
    PageProperties p;
    node_t raw;

    p_converter() : p(){}
    p_converter(node_t raw) : raw(raw) {}
    p_converter(input_vector input, int pageIdx, int treeID, bool feedback) : p(input, pageIdx, treeID, feedback) {}
    
   // p_converter(input_vector input, int pageIdx, int treeID, bool feedback) : p(input, pageIdx, treeID, feedback) {}
};

enum TreeStatus{
    PROCESSING,
    IDLE
};

void feature_distributor(hls::stream<input_vector> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], int size);
void pre_fetcher(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, const int loopCount);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval,100> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut, const int loopCount);
void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval,100> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut, const int loopCount);
void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool, const int loopCount);



#endif