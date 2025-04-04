#include "train.hpp"
#include <hls_np_channel.h>
void rng_splitter(hls::stream<unit_interval> &rngIn, hls::stream<unit_interval> &rngOut);
void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> &rngStream, hls::stream<FetchRequest> &feedbackStream, Page *pageBank1, const int &blockIdx)
{
    #pragma HLS DATAFLOW disable_start_propagation
    hls::stream_of_blocks<IPage, 3> fetchOut[TRAVERSAL_BLOCKS], traverseOut[TRAVERSAL_BLOCKS], nodeSplitOut, pageOut1, pageOut2;
    hls::split::load_balance<unit_interval, TRAVERSAL_BLOCKS, MAX_DEPTH + 2> rngSplitStream;
    // #pragma HLS ARRAY_PARTITION variable=fetchOut[0] dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=fetchOut[1] dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=fetchOut[2] dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=traverseOut[0] dim=1 type=cyclic factor=4
    // #pragma HLS ARRAY_PARTITION variable=traverseOut[1] dim=1 type=cyclic factor=4
    // #pragma HLS ARRAY_PARTITION variable=traverseOut[2] dim=1 type=cyclic factor=4
    // #pragma HLS ARRAY_PARTITION variable=nodeSplitOut dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=pageOut1 dim=1 type=cyclic factor=4 
    // #pragma HLS ARRAY_PARTITION variable=pageOut2 dim=1 type=cyclic factor=4 
    
    rng_splitter(rngStream, rngSplitStream.in);
    pre_fetcher(fetchRequestStream, fetchOut, pageBank1);
    //hls_thread_local hls::task trav[TRAVERSAL_BLOCKS];
    for(int i = 0; i < TRAVERSAL_BLOCKS; i++){
       #pragma HLS UNROLL
        tree_traversal(fetchOut[i], rngSplitStream.out[i], traverseOut[i]);
    }
    
    node_splitter(traverseOut, nodeSplitOut, blockIdx);
    page_splitter(nodeSplitOut, pageOut1, pageOut2);
    save( pageOut1, pageOut2, feedbackStream, pageBank1);
 
}

void rng_splitter(hls::stream<unit_interval> &rngIn, hls::stream<unit_interval> &rngOut)
{
    for(int i = 0; i < MAX_DEPTH*TRAVERSAL_BLOCKS; i++){
        if(!rngOut.full()){
            rngOut.write(rngIn.read());
        }
    }
}