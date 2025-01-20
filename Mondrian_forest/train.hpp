#ifndef TRAIN_HPP
#define TRAIN_HPP
#include "common.hpp"
#include "hls_streamofblocks.h"

struct SplitProperties{
    bool split;
    int nodeIdx;
    int dimension;
    int parentIdx;
    float newSplitTime;

    SplitProperties() : split(false), nodeIdx(0), dimension(0), parentIdx(0), newSplitTime(0) {}
};

struct alignas(128) PageProperties{
    input_vector input;
    int pageIdx;
    int nextPageIdx;
    int freeNodeIdx;
    bool feedback;
    int treeID;
    SplitProperties split;

    PageProperties() : input(), pageIdx(0), nextPageIdx(0), freeNodeIdx(0), feedback(false), treeID(0), split() {}
    PageProperties(input_vector input, int pageIdx, int treeID, bool feedback) : input(input), pageIdx(pageIdx), treeID(treeID), feedback(feedback) {}
};

union p_converter{
    PageProperties p;
    node_t raw;

    p_converter() : p(){}
   // p_converter(input_vector input, int pageIdx, int treeID, bool feedback) : p(input, pageIdx, treeID, feedback) {}
};

enum TreeStatus{
    PROCESSING,
    IDLE
};

void feature_distributor(hls::stream<input_vector> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], int size);
void pre_fetcher(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, int size);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut, int size);
void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut, int size);
void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool, int size);

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const Page *pagePool, bool feedback);

#endif