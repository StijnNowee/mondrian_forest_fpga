#ifndef TRAIN_HPP
#define TRAIN_HPP
#include "common.hpp"
#include <hls_streamofblocks.h>

struct SplitProperties{
    bool enabled;
    int nodeIdx;
    int dimension;
    int parentIdx;
    splitT_t newSplitTime;
    unit_interval rngVal;

    SplitProperties() : enabled(false), nodeIdx(0), dimension(0), parentIdx(0), newSplitTime(0), rngVal(0) {}
    SplitProperties(bool enabled, int nodeIdx, int dimension, int parentIdx, splitT_t newSplitTime, unit_interval rngVal) : enabled(enabled), nodeIdx(nodeIdx), dimension(dimension), parentIdx(parentIdx), newSplitTime(newSplitTime), rngVal(rngVal){}
};

struct alignas(128) PageProperties{
    input_vector input;
    bool needNewPage;
    bool extraPage;
    int pageIdx;
    int nextPageIdx;
    int treeID;
    int freeNodesIdx[2];
    int freePageIdx;
    SplitProperties split;
    

    PageProperties() : input(), pageIdx(0), nextPageIdx(0), freeNodesIdx{-1, -1}, treeID(0), split(), needNewPage(false), extraPage(false), freePageIdx(0) {}
    PageProperties(input_vector input, int pageIdx, int treeID) : input(input), pageIdx(pageIdx), treeID(treeID), freeNodesIdx{-1, -1}, split(), nextPageIdx(0), needNewPage(false), extraPage(false), freePageIdx(0) {}
    void setSplitProperties(int nodeIdx, int dimension, int parentIdx, splitT_t newSplitTime, unit_interval rngVal) {
        split = SplitProperties(true, nodeIdx, dimension, parentIdx, newSplitTime, rngVal);
    }
};


enum TreeStatus{
    IDLE,
    PROCESSING
};

struct PageSplit{
    int bestSplitValue = MAX_NODES_PER_PAGE;
    int bestSplitLocation = 0;
    int nrOfBranchedNodes = 0;
    int freePageIndex;
};
void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS], hls::stream<FetchRequest> &feedbackStream, Page *pageBank1, hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream, const int size);

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> pageOut[TRAVERSAL_BLOCKS], const Page *pagePool, hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &rngStream, hls::stream_of_blocks<IPage> &pageOut);
void page_splitter(hls::stream_of_blocks<IPage> pageIn[TRAVERSAL_BLOCKS], hls::stream_of_blocks<IPage> &pageOut);
void node_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream_of_blocks<IPage> &pageOut);
void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool, const int size);

void write_page(const IPage &localPage, const PageProperties &p, hls::stream_of_blocks<IPage> &pageOut);
void read_page(IPage &localPage, PageProperties &p, hls::stream_of_blocks<IPage> &pageIn);



#endif