#ifndef TRAIN_HPP
#define TRAIN_HPP
#include "common.hpp"
#include <hls_stream.h>
#include <hls_streamofblocks.h>


struct PageSplit{
    int bestSplitValue = MAX_NODES_PER_PAGE;
    int bestSplitLocation = 0;
    int nrOfBranchedNodes = 0;
};
void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> &rngStream, hls::stream<Feedback> &feedbackStream, Page *pageBank1, const int &blockIdx);

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> pageOut[TRAVERSAL_BLOCKS], const Page *pagePool);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &rngStream, hls::stream_of_blocks<IPage> &pageOut);
void node_splitter(hls::stream_of_blocks<IPage> pageIn[TRAVERSAL_BLOCKS], hls::stream_of_blocks<IPage> &pageOut, const int &blockIdx);
void page_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream_of_blocks<IPage> &pageOut1, hls::stream_of_blocks<IPage> &pageOut2);
void save(hls::stream_of_blocks<IPage> &pageIn1, hls::stream_of_blocks<IPage> &pageIn2, hls::stream<Feedback> &feedbackStream, Page *pagePool);


#endif