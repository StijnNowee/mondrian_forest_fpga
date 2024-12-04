#ifndef TRAIN_HPP
#define TRAIN_HPP
#include "control_unit.hpp"
#include <variant>

void train(
    hls::stream<FetchRequest> &fetchRequestStream,
    hls::stream<unit_interval> &traversalRNGStream,
    hls::stream<unit_interval> &splitterRNGStream,
    hls::stream<FetchRequest> &outputRequestStream,
    Page* pagePool_read,
    Page* pagePool_write
);

struct PageProperties{
    feature_vector feature;
    int pageIdx;
    int nextPageIdx;
    bool split = false;
    int splitIdx;
    int freeNodeIdx = 0;
    int rootNodeIdx = 0;
};

struct PageChunk{
    //std::variant<Node_hbm, PageProperties> data;
    union{
        Node_hbm node;
        PageProperties p;
    };
    PageChunk(){}
    PageChunk(const Node_hbm& n) : node(n) {}
    PageChunk(const PageProperties& p) : p(p) {}
};

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<PageChunk> &pageOut, const Page *pagePool);
void tree_traversal(hls::stream<PageChunk> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream<PageChunk> &pageOut);
void splitter(hls::stream<PageChunk> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream<PageChunk> &pageOut);
void save(hls::stream<PageChunk> &pageIn, hls::stream<FetchRequest> &request, Page *pagePool);

#endif