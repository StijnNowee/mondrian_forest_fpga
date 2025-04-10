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
    
    fetcher<TRAIN_TRAVERSAL_BLOCKS, FetchRequest, PageProperties>(fetchRequestStream, fetchOut, pageBank);
    for(int i = 0; i < TRAIN_TRAVERSAL_BLOCKS; i++){
       #pragma HLS UNROLL
        tree_traversal(fetchOut[i], rngStream[i], traverseOut[i]);
    }
    
    node_splitter(traverseOut, nodeSplitOut, blockIdx);
    page_splitter(nodeSplitOut, pageOut1, pageOut2);
    save( pageOut1, pageOut2, feedbackStream, pageBank);
 
}