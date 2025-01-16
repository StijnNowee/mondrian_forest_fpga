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

struct alignas(128) PageProperties{
    input_vector input;
    int pageIdx;
    int nextPageIdx = 0;
    int freeNodeIdx;
    SplitProperties split;
};

union p_converter{
    PageProperties p;
    node_t raw;

    p_converter(){}
};

//typedef ap_uint<1024> PageChunk;

//typedef ap_uint<1024> Page[MAX_NODES_PER_PAGE + 1];

//void pre_fetcher_old(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<Page> &pageOut, const Page *pagePool);


void pre_fetcher(hls::stream<input_vector> &newFeatureStream, hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, int size);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut, int size);
void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut, int size);
void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool, int size);

//void burst_read_page(int pageIdx, const input_vector &input, volatile const Page *pagePool, Page pageOut);
//void read_internal_page(Page pageIn, Page pageOut);

#endif