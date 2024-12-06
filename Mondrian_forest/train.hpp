#ifndef TRAIN_HPP
#define TRAIN_HPP
#include "control_unit.hpp"
#include <variant>
#include "hls_streamofblocks.h"

void train(
    hls::stream<FetchRequest> &fetchRequestStream,
    hls::stream<unit_interval> &traversalRNGStream,
    hls::stream<unit_interval> &splitterRNGStream,
    hls::stream<FetchRequest> &outputRequestStream,
    const Page* pagePool_read,
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

struct alignas(128) PageChunk{
    union{
        Node_hbm node;
        PageProperties p;
    };
    PageChunk() : node() {}
    // PageChunk(){}
    // PageChunk(const Node_hbm& n) : node(n) {}
    // PageChunk(const PageProperties& p) : p(p) {}

    // PageChunk& operator=(const PageChunk& other) {
    //     if (this != &other) {
    //         if (&node == &other.node) {
    //             node = other.node;
    //         } else if (&p == &other.p) {
    //             p = other.p;
    //         }
    //     }
    //     return *this;
    // }
};

typedef PageChunk IPage[MAX_NODES_PER_PAGE + 1];

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool);
void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut);
void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut);
void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &request);

#endif