#ifndef TRAIN_HPP
#define TRAIN_HPP
#include <variant>
#include "common.hpp"
#include "hls_streamofblocks.h"


struct PageProperties{
    feature_vector feature;
    int pageIdx;
    int nextPageIdx;
    bool split = false;
    int splitIdx;
    int freeNodeIdx = 0;
    int rootNodeIdx = 0;
};

typedef ap_uint<1024> PageChunk;

typedef PageChunk IPage[MAX_NODES_PER_PAGE + 1];

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut);
void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut);
void save(hls::stream_of_blocks<IPage> &pageIn, FetchRequest &feedbackRegister, Page *pagePool);//hls::stream<FetchRequest> &feedbackStream,

#endif