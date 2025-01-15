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

//typedef ap_uint<1024> PageChunk;

//typedef ap_uint<1024> Page[MAX_NODES_PER_PAGE + 1];

//void pre_fetcher_old(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<Page> &pageOut, const Page *pagePool);


void pre_fetcher(hls::stream<input_vector> &newFeatureStream, hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<Page> &pageOut, hls::stream<PageProperties> &traversalControl, const Page *pagePool);
void tree_traversal(hls::stream_of_blocks<Page> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<Page> &pageOut, hls::stream<PageProperties> &control, hls::stream<PageProperties> &splitterControl);
void splitter(hls::stream_of_blocks<Page> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<Page> &pageOut, hls::stream<PageProperties> &control, hls::stream<PageProperties> &saveControl);
void save(hls::stream_of_blocks<Page> &pageIn, hls::stream<FetchRequest> &feedbackStream, hls::stream<PageProperties> &control, Page *pagePool);

//void burst_read_page(int pageIdx, const input_vector &input, volatile const Page *pagePool, Page pageOut);
//void read_internal_page(Page pageIn, Page pageOut);

#endif