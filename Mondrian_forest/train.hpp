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

    SplitProperties() : enabled(false), nodeIdx(0), dimension(0), parentIdx(0), newSplitTime(0) {}
    SplitProperties(bool enabled, int nodeIdx, int dimension, int parentIdx, splitT_t newSplitTime) : enabled(enabled), nodeIdx(nodeIdx), dimension(dimension), parentIdx(parentIdx), newSplitTime(newSplitTime) {}
};

struct alignas(128) PageProperties{
    input_vector input;
    bool needNewPage;
    bool extraPage;
    int pageIdx;
    int nextPageIdx;
    int treeID;
    int freeNodesIdx[2];
    SplitProperties split;
    

    PageProperties() : input(), pageIdx(0), nextPageIdx(0), freeNodesIdx{-1, -1}, treeID(0), split(), needNewPage(false), extraPage(false) {}
    PageProperties(input_vector input, int pageIdx, int treeID) : input(input), pageIdx(pageIdx), treeID(treeID), freeNodesIdx{-1, -1}, split(), nextPageIdx(0), needNewPage(false), extraPage(false) {}
    void setSplitProperties(int nodeIdx, int dimension, int parentIdx, splitT_t newSplitTime) {
        split = SplitProperties(true, nodeIdx, dimension, parentIdx, newSplitTime);
    }
};

void convertPropertiesToRaw(const PageProperties &p, node_t &raw);
void convertRawToProperties(const node_t &raw, PageProperties &p);

void convertNodeToRaw(const Node_hbm &node, node_t &raw);
void convertRawToNode(const node_t &raw, Node_hbm &node);


enum TreeStatus{
    IDLE,
    PROCESSING,
    UPDATING
};

struct PageSplit{
    int bestSplitValue = MAX_NODES_PER_PAGE;
    int bestSplitLocation = 0;
    int nrOfBranchedNodes = 0;
    int freePageIndex = 0;
};
void train(hls::stream<input_t> &inputFeatureStream, hls::stream<node_t> &outputStream, hls::stream<ap_uint<72>> &smlNodeOutputStream, hls::stream<bool> &controlOutputStream, Page *pageBank1, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<bool> &treeUpdateCtrlStream, const int size);

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], const int size);
void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, const int size);
void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<ap_uint<72>> &smlNodeOutputStream,  hls::stream<bool> &treeUpdateCtrlStream);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut);
void page_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream_of_blocks<IPage> &pageOut);
void node_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut);
void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, hls::stream<bool> &controlOutputStream, Page *pagePool);

void write_page(const IPage &localPage, const PageProperties &p, hls::stream_of_blocks<IPage> &pageOut);
void read_page(IPage &localPage, PageProperties &p, hls::stream_of_blocks<IPage> &pageIn);



#endif