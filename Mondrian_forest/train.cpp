#include "train.hpp"
#include "hls_task.h"
#include "rng.hpp"

void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> &rngStream, hls::stream<FetchRequest> &feedbackStream, Page *pageBank1, hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream, const int size)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE port=return mode=ap_ctrl_chain

    hls_thread_local hls::stream_of_blocks<IPage,10> fetchOutput;
    hls_thread_local hls::stream_of_blocks<IPage,3> traverseOutput;
    hls_thread_local hls::stream_of_blocks<IPage,3> pageSplitterOut;
    hls_thread_local hls::stream_of_blocks<IPage,3> nodeSplitterOut;

    //hls_thread_local hls::task t2(feature_distributor, inputFeatureStream, splitFeatureStream);
    pre_fetcher(fetchRequestStream, fetchOutput, pageBank1, smlTreeStream, treeUpdateCtrlStream);
    //hls_thread_local hls::task t1(rng_generator, rngStream.in);
    hls_thread_local hls::task t3(tree_traversal, fetchOutput, rngStream, traverseOutput);
    hls_thread_local hls::task t4(page_splitter,traverseOutput, pageSplitterOut);
    hls_thread_local hls::task t5(node_splitter,pageSplitterOut, nodeSplitterOut);
    save( nodeSplitterOut, feedbackStream, pageBank1, size);
 
}



// void convertNodeToRaw(const Node_hbm &node, node_t &raw){
//     raw = *reinterpret_cast<const node_t*>(&node);
// }
// void convertRawToNode(const node_t &raw, Node_hbm &node){
//     *reinterpret_cast<node_t*>(&node) = raw;
// }

// void convertPropertiesToRaw(const PageProperties &p, node_t &raw){
//     raw = *reinterpret_cast<const node_t*>(&p);
// }
// void convertRawToProperties(const node_t &raw, PageProperties &p){
//     *reinterpret_cast<node_t*>(&p) = raw;
// }

void write_page(const IPage &localPage, const PageProperties &p, hls::stream_of_blocks<IPage> &pageOut){
    #pragma HLS INLINE off
    hls::write_lock<IPage> out(pageOut);
    for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        out[n] = localPage[n];
    }
    convertPropertiesToRaw(p, out[MAX_NODES_PER_PAGE]);
}
void read_page(IPage &localPage, PageProperties &p, hls::stream_of_blocks<IPage> &pageIn){
    #pragma HLS INLINE off
    hls::read_lock<IPage> in(pageIn);
    for(int n = 0; n < MAX_NODES_PER_PAGE + 1; n++){
        localPage[n] = in[n];
    }
    convertRawToProperties(in[MAX_NODES_PER_PAGE], p);
}

void convertNodeToRaw(const Node_hbm &node, node_t &raw){
    #pragma HLS INLINE
    int bit_offset = 0;

    // int idx;
    raw.range(bit_offset + 31, bit_offset) = node.idx;
    bit_offset += 32;
    // bool leaf;
    raw.range(bit_offset, bit_offset) = node.leaf;
    bit_offset += 1;
    // bool valid;
    raw.range(bit_offset, bit_offset) = node.valid;
    bit_offset += 1;
    // ap_uint feature;
    raw.range(bit_offset + 7, bit_offset) = node.feature;
    bit_offset += 8;
    // unit_interval threshold;
    raw.range(bit_offset + 7, bit_offset) = node.threshold.range(7, 0);
    bit_offset += 8;
    // splitT_t splittime;
    raw.range(bit_offset + 23, bit_offset) = node.splittime.range(23, 0);
    bit_offset += 24;
    // splitT_t parentSplitTime;
    raw.range(bit_offset + 23, bit_offset) = node.parentSplitTime.range(23, 0);
    bit_offset += 24;
    // feature_vector lowerBound;
    for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
        raw.range(bit_offset + 7, bit_offset) = node.lowerBound[i].range(7, 0);
        bit_offset += 8;
    }
    // feature_vector upperBound;
    for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
        raw.range(bit_offset + 7, bit_offset) = node.upperBound[i].range(7, 0);
        bit_offset += 8;
    }
    // int labelCount;
    raw.range(bit_offset + 31, bit_offset) = node.labelCount;
    bit_offset += 32;
    // classDistribution_t classDistribution;
    for (int i = 0; i < CLASS_COUNT; i++) {
        raw.range(bit_offset + 8, bit_offset) = node.classDistribution[i].range(8, 0);
        bit_offset += 9;
    }
    // ChildNode leftChild;
    raw.range(bit_offset, bit_offset) = node.leftChild.isPage;
    bit_offset += 1;
    raw.range(bit_offset + 31, bit_offset) = node.leftChild.id;
    bit_offset += 32;

    // ChildNode rightChild;
    raw.range(bit_offset, bit_offset) = node.rightChild.isPage;
    bit_offset += 1;
    raw.range(bit_offset + 31, bit_offset) = node.rightChild.id;
    bit_offset += 32;
    
    //Fill with 0
    raw.range(1023, bit_offset) = 0;
}
void convertRawToNode(const node_t &raw, Node_hbm &node){
    #pragma HLS INLINE
    int bit_offset = 0;

    // int idx;
    node.idx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;
    // bool leaf;
    node.leaf = raw.range(bit_offset, bit_offset);
    bit_offset += 1;
    // bool valid;
    node.valid = raw.range(bit_offset, bit_offset);
    bit_offset += 1;
    // ap_uint feature;
    node.feature = raw.range(bit_offset + 7, bit_offset);
    bit_offset += 8;
    // unit_interval threshold;
    node.threshold.range(7, 0) = raw.range(bit_offset + 7, bit_offset);
    bit_offset += 8;
    // splitT_t splittime;
    node.splittime.range(23, 0) = raw.range(bit_offset + 23, bit_offset);
    bit_offset += 24;
    // splitT_t parentSplitTime;
    node.parentSplitTime.range(23, 0) = raw.range(bit_offset + 23, bit_offset);
    bit_offset += 24;
    // feature_vector lowerBound;
    for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
        node.lowerBound[i].range(7, 0) = raw.range(bit_offset + 7, bit_offset);
        bit_offset += 8;
    }
    // feature_vector upperBound;
    for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
        node.upperBound[i].range(7, 0) = raw.range(bit_offset + 7, bit_offset);
        bit_offset += 8;
    }
    // int labelCount;
    node.labelCount = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;
    // classDistribution_t classDistribution;
    for (int i = 0; i < CLASS_COUNT; i++) {
        node.classDistribution[i].range(8, 0) = raw.range(bit_offset + 8, bit_offset);
        bit_offset += 9;
    }
    // ChildNode leftChild;
    node.leftChild.isPage = raw.range(bit_offset, bit_offset);
    bit_offset += 1;
    node.leftChild.id = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;
    // ChildNode rightChild;
    node.rightChild.isPage = raw.range(bit_offset, bit_offset);
    bit_offset += 1;
    node.rightChild.id = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;
}

void convertPropertiesToRaw(const PageProperties &p, node_t &raw){
    #pragma HLS INLINE
    raw = 0;
    int bit_offset = 0;

    // input_vector input;
    for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
        raw.range(bit_offset + 7, bit_offset) = p.input.feature[i].range(7,0);
        bit_offset += 8;
    }
    raw.range(bit_offset + CLASS_BITS - 1, bit_offset) = p.input.label;
    bit_offset += CLASS_BITS;
    raw.range(bit_offset, bit_offset) = p.input.inferenceSample;
    bit_offset += 1;

    // bool needNewPage;
    raw.range(bit_offset, bit_offset) = p.needNewPage;
    bit_offset += 1;

    // bool extraPage;
    raw.range(bit_offset, bit_offset) = p.extraPage;
    bit_offset += 1;

    // int pageIdx;
    raw.range(bit_offset + 31, bit_offset) = p.pageIdx;
    bit_offset += 32;

    // int nextPageIdx;
    raw.range(bit_offset + 31, bit_offset) = p.nextPageIdx;
    bit_offset += 32;

    // int treeID;
    raw.range(bit_offset + 31, bit_offset) = p.treeID;
    bit_offset += 32;

    // int freeNodesIdx[2];
    for (int i = 0; i < 2; i++) {
        raw.range(bit_offset + 31, bit_offset) = p.freeNodesIdx[i];
        bit_offset += 32;
    }

    // int freePageIdx;
    raw.range(bit_offset + 31, bit_offset) = p.freePageIdx;
    bit_offset += 32;

    // SplitProperties split;
    raw.range(bit_offset, bit_offset) = p.split.enabled;
    bit_offset += 1;
    raw.range(bit_offset + 31, bit_offset) = p.split.nodeIdx;
    bit_offset += 32;
    raw.range(bit_offset + 31, bit_offset) = p.split.dimension;
    bit_offset += 32;
    raw.range(bit_offset + 31, bit_offset) = p.split.parentIdx;
    bit_offset += 32;
    raw.range(bit_offset + 23, bit_offset) = p.split.newSplitTime.range(23,0);
    bit_offset += 24;
    raw.range(bit_offset + 7, bit_offset) = p.split.rngVal.range(7,0);
    bit_offset += 8;
    
    //raw.range(1023, bit_offset) = 0;
}
void convertRawToProperties(const node_t &raw, PageProperties &p){
    #pragma HLS INLINE
    int bit_offset = 0;

    // input_vector input;
    for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
        p.input.feature[i].range(7,0) = raw.range(bit_offset + 7, bit_offset);
        bit_offset += 8;
    }
    p.input.label = raw.range(bit_offset + CLASS_BITS - 1, bit_offset);
    bit_offset += CLASS_BITS;
    p.input.inferenceSample = raw.range(bit_offset, bit_offset);
    bit_offset += 1;

    // bool needNewPage;
    p.needNewPage = raw.range(bit_offset, bit_offset);
    bit_offset += 1;

    // bool extraPage;
    p.extraPage = raw.range(bit_offset, bit_offset);
    bit_offset += 1;

    // int pageIdx;
    p.pageIdx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;

    // int nextPageIdx;
    p.nextPageIdx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;

    // int treeID;
    p.treeID = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;

    // int freeNodesIdx[2];
    for (int i = 0; i < 2; i++) {
        p.freeNodesIdx[i] = raw.range(bit_offset + 31, bit_offset);
        bit_offset += 32;
    }

    // int freePageIdx;
    p.freePageIdx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;

    // SplitProperties split;
    p.split.enabled = raw.range(bit_offset, bit_offset);
    bit_offset += 1;
    p.split.nodeIdx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;
    p.split.dimension = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;
    p.split.parentIdx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;
    p.split.newSplitTime.range(23, 0) = raw.range(bit_offset + 23, bit_offset);
    bit_offset += 24;
    p.split.rngVal.range(7,0) = raw.range(bit_offset + 7, bit_offset);
    bit_offset += 8;
}