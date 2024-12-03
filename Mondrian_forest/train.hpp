#ifndef TRAIN_HPP
#define TRAIN_HPP
#include "control_unit.hpp"

void train(
    hls::stream<FetchRequest> &fetchRequestStream,
    hls::stream<unit_interval> &traversalRNGStream,
    hls::stream<unit_interval> &splitterRNGStream,
    hls::stream<FetchRequest> &outputRequestStream,
    Page* pagePool
);

struct InternalPage{
    Page page;
    feature_vector feature;
    int pageIdx;
    int nextPageIdx;
    bool split = false;
    int splitIdx;
};

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<InternalPage> &pageOut, Page *pagePool);
void tree_traversal(hls::stream<InternalPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream<InternalPage> &pageOut);
void splitter(hls::stream<InternalPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream<InternalPage> &pageOut);
void save(hls::stream<InternalPage> &pageIn, hls::stream<FetchRequest> &request, Page *pagePool);

#endif