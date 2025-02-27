#include "train.hpp"
#include <hls_np_channel.h>
#include "hls_task.h"
#include "rng.hpp"
void train(hls::stream<input_t> &inputFeatureStream, hls::stream<node_t> &outputStream, hls::stream<bool> &controlOutputStream,Page *pageBank1)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_ctrl_none port=return
    hls_thread_local hls::stream<FetchRequest,5> feedbackStream("FeedbackStream");


    hls_thread_local hls::stream_of_blocks<IPage,10> fetchOutput;
    hls_thread_local hls::stream_of_blocks<IPage,3> traverseOutput;
    hls_thread_local hls::stream_of_blocks<IPage,3> pageSplitterOut;
    hls_thread_local hls::stream_of_blocks<IPage,3> nodeSplitterOut;

    hls_thread_local hls::stream<input_vector,1> splitFeatureStream[TREES_PER_BANK];

    
    //hls_thread_local hls::stream<unit_interval, 100> rngStream[2*BANK_COUNT];
    hls_thread_local hls::split::load_balance<unit_interval, 2> rngStream;

    hls_thread_local hls::task t1(rng_generator, rngStream.in);
    hls_thread_local hls::task t2(feature_distributor, inputFeatureStream, splitFeatureStream);
    //hls_thread_local hls::task t2(feature_distributor, inputFeatureStream, splitFeatureStream);
    hls_thread_local hls::task t7(pre_fetcher, splitFeatureStream, feedbackStream, fetchOutput, pageBank1);
    hls_thread_local hls::task t3(tree_traversal, fetchOutput, rngStream.out[0], traverseOutput);
    hls_thread_local hls::task t4(page_splitter,traverseOutput, pageSplitterOut);
    hls_thread_local hls::task t5(node_splitter,pageSplitterOut, rngStream.out[1], nodeSplitterOut);
    hls_thread_local hls::task t6(save, nodeSplitterOut, feedbackStream, outputStream, controlOutputStream, pageBank1);
}

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK])
{
    auto rawInput = newFeatureStream.read();
    input_vector newInput;
    newInput.label = rawInput.range(CLASS_BITS-1, 0);
    for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
        newInput.feature[i].range(7,0) = rawInput.range(CLASS_BITS-1 + 8*(i+1), CLASS_BITS + 8*i);
    }

    for(int t = 0; t < TREES_PER_BANK; t++){

        splitFeatureStream[t].write(newInput);
    }
}

// void convertPropertiesToRaw(const PageProperties &p, node_t &raw)
// {
//     raw = 0;
//     raw.range(31, 0) = p.pageIdx;
//     raw.range(63, 32) = p.nextPageIdx;
//     raw.range(95, 64) = p.treeID;
//     raw.range(127, 96) = p.freeNodesIdx[0];
//     raw.range(159, 128) = p.freeNodesIdx[1];
//     raw.range(160, 160) = p.split.enabled;
//     raw.range(192, 161) = p.split.nodeIdx;
//     raw.range(224, 193) = p.split.dimension;
//     raw.range(256, 225) = p.split.parentIdx;
//     raw.range(280, 257) = p.split.newSplitTime.range(23, 0);
//     raw.range(281, 281) = p.needNewPage;
//     raw.range(282,282) = p.extraPage; 
//     raw.range(282 + CLASS_BITS,283) = p.input.label;
//     for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
//         #pragma HLS UNROLL
//         raw.range(290 + CLASS_BITS + i*8, 283 + CLASS_BITS + i*8) = p.input.feature[i].range(7,0);
//     }
// }

// void convertRawToProperties(const node_t &raw, PageProperties &p)
// {
//     p.pageIdx =             raw.range(31, 0);
//     p.nextPageIdx =         raw.range(63, 32);
//     p.treeID =              raw.range(95, 64);
//     p.freeNodesIdx[0] =     raw.range(127, 96);
//     p.freeNodesIdx[1] =     raw.range(159, 128);
//     p.split.enabled =       raw.range(160, 160);
//     p.split.nodeIdx =       raw.range(192, 161);
//     p.split.dimension =     raw.range(224, 193);
//     p.split.parentIdx =     raw.range(256, 225);
//     p.split.newSplitTime.range(23, 0) =  raw.range(280, 257);
//     p.needNewPage =         raw.range(281, 281);
//     p.extraPage =           raw.range(282,282);
//     p.input.label =         raw.range(282 + CLASS_BITS,283);
    
//     for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
//         #pragma HLS UNROLL
//         p.input.feature[i].range(7,0) = raw.range(290 + CLASS_BITS + i*8, 283 + CLASS_BITS + i*8);
//     }
// }

// void convertNodeToRaw(const Node_hbm &node, node_t &raw)
// {
//     int tmp1 = node.leftChild.id;
//     int tmp2 = node.rightChild.id;
//     raw.range(31, 0) = node.idx;
//     raw.range(32, 32) = node.leaf;
//     raw.range(33, 33) = node.valid; //If this is changed, please change it in findFreeNodes and pageSplit
//     raw.range(41, 34) = node.feature;
//     raw.range(49,42) = node.threshold.range(7,0);
//     raw.range(73,50) = node.splittime.range(23, 0);
//     raw.range(97,74) = node.parentSplitTime.range(23, 0);
//     raw.range(129,98) = tmp1;
//     raw.range(130,130) = node.leftChild.isPage;
//     raw.range(162,131) = tmp2;
//     raw.range(163, 163) = node.rightChild.isPage;
//     raw.range(195, 164) = node.labelCount;
//     for(int i = 0; i < CLASS_COUNT; i++){
//         #pragma HLS UNROLL
//         raw.range(204 + i*9,196 + i*9) = node.classDistribution[i].range(8, 0);
//     }
//     int baseAddress = 205 + 9*(CLASS_COUNT-1);
//     for(int j = 0; j < FEATURE_COUNT_TOTAL; j++){
//         #pragma HLS UNROLL
//         raw.range(baseAddress + 7 + j*8,baseAddress + j*8) = node.lowerBound[j].range(7,0);
//         raw.range(baseAddress + 7 + j*8 + FEATURE_COUNT_TOTAL*8, baseAddress + j*8 + FEATURE_COUNT_TOTAL*8) = node.upperBound[j].range(7,0);
//     }
// }

// void convertRawToNode(const node_t &raw, Node_hbm &node)
// {
//     node.idx = raw.range(31, 0);
//     node.leaf = raw.range(32, 32);
//     node.valid = raw.range(33, 33);
//     node.feature = raw.range(41, 34);
//     node.threshold.range(7,0) = raw.range(49,42);
//     node.splittime.range(23, 0) = raw.range(73,50);
//     node.parentSplitTime.range(23, 0) = raw.range(97,74);
//     node.leftChild.id = raw.range(129,98);
//     node.leftChild.isPage = raw.range(130,130);
//     node.rightChild.id = raw.range(162,131);
//     node.rightChild.isPage = raw.range(163, 163);
//     node.labelCount = raw.range(195, 164);
//     for(int i = 0; i < CLASS_COUNT; i++){
//         #pragma HLS UNROLL
//         node.classDistribution[i].range(8,0) = raw.range(204 + i*9,196 + i*9);
//     }
//     int baseAddress = 205 + 9*(CLASS_COUNT-1);
//     for(int j = 0; j < FEATURE_COUNT_TOTAL; j++){
//         #pragma HLS UNROLL
//         node.lowerBound[j].range(7,0) = raw.range(baseAddress + 7 + j*8,baseAddress + j*8);
//         node.upperBound[j].range(7,0) = raw.range(baseAddress + 7 + j*8 + FEATURE_COUNT_TOTAL*8, baseAddress + j*8 + FEATURE_COUNT_TOTAL*8);
//     }
// }

void convertNodeToRaw(const Node_hbm &node, node_t &raw){
    raw = *reinterpret_cast<const node_t*>(&node);
}
void convertRawToNode(const node_t &raw, Node_hbm &node){
    *reinterpret_cast<node_t*>(&node) = raw;
}

void convertPropertiesToRaw(const PageProperties &p, node_t &raw){
    raw = *reinterpret_cast<const node_t*>(&p);
}
void convertRawToProperties(const node_t &raw, PageProperties &p){
    *reinterpret_cast<node_t*>(&p) = raw;
}