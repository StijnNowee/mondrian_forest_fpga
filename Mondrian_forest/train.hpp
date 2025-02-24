#ifndef TRAIN_HPP
#define TRAIN_HPP
#include "common.hpp"
#include "hls_streamofblocks.h"

struct SplitProperties{
    bool enabled;
    int nodeIdx;
    int dimension;
    int parentIdx;
    splitT_t newSplitTime;

    SplitProperties() : enabled(false), nodeIdx(0), dimension(0), parentIdx(0), newSplitTime(0) {}
    SplitProperties(bool enabled, int nodeIdx, int dimension, int parentIdx, splitT_t newSplitTime) : enabled(enabled), nodeIdx(nodeIdx), dimension(dimension), parentIdx(parentIdx), newSplitTime(newSplitTime) {}
};

struct PageProperties{
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

node_t convertProperties(const PageProperties &p);
PageProperties convertProperties(const node_t &raw);

void convertNodeToRaw(const Node_hbm &node, node_t &raw);
void convertRawToNode(const node_t &raw, Node_hbm &node);


enum TreeStatus{
    TRAINING,
    IDLE
};

struct PageSplit{
    int bestSplitValue = MAX_NODES_PER_PAGE;
    int bestSplitLocation = 0;
    int nrOfBranchedNodes = 0;
    int freePageIndex = 0;
};
//void train(hls::stream<input_vector> &trainInputStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream, hls::stream<node_t> &outputStream, Page *pageBank1);
void train(hls::stream<input_vector> &trainInputStream, hls::stream<int> &processTreeStream, hls::stream<node_t> &outputStream, Page *pageBank1);

void feature_distributor(hls::stream<input_vector> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK]);
//void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream);
void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, hls::stream<int> &processTreeStream);
void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut);
void page_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream_of_blocks<IPage> &pageOut);
void node_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut);
void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, hls::stream<node_t> &outputStream, Page *pagePool);



#endif