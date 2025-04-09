#include "train.hpp"
#include "processing_unit.hpp"
#include <hls_math.h>
void rng_splitter(hls::stream<unit_interval> &rngIn, hls::stream<unit_interval> &rngOut);
void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], hls::stream<Feedback> &feedbackStream, PageBank &pageBank, const int &blockIdx)
{
    #pragma HLS DATAFLOW disable_start_propagation
    hls::stream_of_blocks<IPage, 3> fetchOut[TRAIN_TRAVERSAL_BLOCKS], traverseOut[TRAIN_TRAVERSAL_BLOCKS], nodeSplitOut, pageOut1, pageOut2;
    // #pragma HLS ARRAY_PARTITION variable=fetchOut[0] dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=fetchOut[1] dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=fetchOut[2] dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=traverseOut[0] dim=1 type=cyclic factor=4
    // #pragma HLS ARRAY_PARTITION variable=traverseOut[1] dim=1 type=cyclic factor=4
    // #pragma HLS ARRAY_PARTITION variable=traverseOut[2] dim=1 type=cyclic factor=4
    // #pragma HLS ARRAY_PARTITION variable=nodeSplitOut dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=pageOut1 dim=1 type=cyclic factor=4 
    // #pragma HLS ARRAY_PARTITION variable=pageOut2 dim=1 type=cyclic factor=4 
    
    fetcher<TRAIN_TRAVERSAL_BLOCKS>(fetchRequestStream, fetchOut, pageBank);
    for(int i = 0; i < TRAIN_TRAVERSAL_BLOCKS; i++){
       #pragma HLS UNROLL
        tree_traversal(fetchOut[i], rngStream[i], traverseOut[i]);
    }
    
    node_splitter(traverseOut, nodeSplitOut, blockIdx);
    page_splitter(nodeSplitOut, pageOut1, pageOut2);
    save( pageOut1, pageOut2, feedbackStream, pageBank);
 
}

void update_internal_posterior_predictive_distribution(Node_hbm &node, const posterior_t &parentG)
{
    
    auto test = -GAMMA*(node.splittime - node.parentSplitTime);
    ap_ufixed<32, 0> discount = hls::exp(test.to_float());
    int totalCount = 0;
    int countPerClass[CLASS_COUNT];
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        countPerClass[c] = node.getTab(LEFT, c) + node.getTab(RIGHT, c);
        totalCount += countPerClass[c];
    }
    ap_ufixed<32, 1> oneoverCount = 1.0/totalCount;
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        node.posteriorP[c] = oneoverCount*(countPerClass[c] - discount*(node.counts[c] > 0) + discount*totalCount*parentG[c]);
    }
}

void update_leaf_posterior_predictive_distribution(Node_hbm &node, const posterior_t &parentG)
{
    auto test = -GAMMA*(node.splittime - node.parentSplitTime);
    ap_ufixed<32, 0> discount = hls::exp(test.to_float());

    int totalCount = 0;
    int totalTabs = 0;
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        totalCount += node.counts[c];
        totalTabs += node.counts[c] > 0; 
    }
    ap_ufixed<32, 1> oneoverCount = 1.0/totalCount;
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        node.posteriorP[c] = oneoverCount*(node.counts[c] - discount*(node.counts[c] > 0) + discount*totalTabs*parentG[c]);
    }
}