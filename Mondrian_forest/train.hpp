#ifndef TRAIN_HPP
#define TRAIN_HPP
#include "common.hpp"
#include "hls_streamofblocks.h"

struct SplitProperties{
    bool split = false;
    int nodeIdx;
    int dimension;
    int parentIdx;
    float newSplitTime;
};

struct PageProperties{
    input_vector input;
    int pageIdx;
    int nextPageIdx = 0;
    int freeNodeIdx;
    SplitProperties split;
};

typedef ap_uint<1024> PageChunk;

typedef PageChunk IPage[MAX_NODES_PER_PAGE + 1];

void pre_fetcher_old(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool);

void pre_fetcher(hls::stream<input_vector> &newFeatureStream, hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut);
void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut);
void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool);//hls::stream<FetchRequest> &feedbackStream,

void burst_read_page(int pageIdx, const input_vector &input, const Page *pagePool, hls::stream_of_blocks<IPage> &pageOut);

#endif