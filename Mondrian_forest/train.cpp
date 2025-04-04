#include "train.hpp"
#include "converters.hpp"
void traverseBlocks(hls::stream_of_blocks<IPage> &fetchOut, hls::stream_of_blocks<IPage> &traverseOut);
void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS], hls::stream<FetchRequest> &feedbackStream, Page *pageBank1, const int &id, const int &blockIdx)
{
    #pragma HLS DATAFLOW disable_start_propagation
    hls::stream_of_blocks<IPage, 3> fetchOut[TRAVERSAL_BLOCKS], traverseOut[TRAVERSAL_BLOCKS], nodeSplitOut, pageOut1, pageOut2;
    // #pragma HLS ARRAY_PARTITION variable=fetchOut[0] dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=fetchOut[1] dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=fetchOut[2] dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=traverseOut[0] dim=1 type=cyclic factor=4
    // #pragma HLS ARRAY_PARTITION variable=traverseOut[1] dim=1 type=cyclic factor=4
    // #pragma HLS ARRAY_PARTITION variable=traverseOut[2] dim=1 type=cyclic factor=4
    // #pragma HLS ARRAY_PARTITION variable=nodeSplitOut dim=1 type=block factor=4
    // #pragma HLS ARRAY_PARTITION variable=pageOut1 dim=1 type=cyclic factor=4 
    // #pragma HLS ARRAY_PARTITION variable=pageOut2 dim=1 type=cyclic factor=4 
    
    

    // #pragma HLS BIND_STORAGE variable=fetchOut type=ram_2p impl=uram
    // #pragma HLS BIND_STORAGE variable=traverseOut type=ram_2p impl=uram
    // #pragma HLS BIND_STORAGE variable=pageOut1 type=ram_2p impl=uram
    // #pragma HLS BIND_STORAGE variable=pageOut2 type=ram_2p impl=uram
    // #pragma HLS BIND_STORAGE variable=nodeSplitOut type=ram_2p impl=uram


    pre_fetcher(fetchRequestStream, fetchOut, pageBank1, blockIdx);
    //hls_thread_local hls::task trav[TRAVERSAL_BLOCKS];
    for(int i = 0; i < TRAVERSAL_BLOCKS; i++){
       #pragma HLS UNROLL
        tree_traversal(fetchOut[i], rngStream[i + TRAVERSAL_BLOCKS*id], traverseOut[i]);
    }
    
    
    //traverseBlocks(fetchOut, traverseOut);
    // #ifdef __SYNTHESIS__
    // hls_thread_local hls::task t1(tree_traversal,fetchOut[0], rngStream[0 + id*3], traverseOut[0]);
    // #else
    //tree_traversal(fetchOut[0], traverseOut[0]);
    // #endif
    // hls_thread_local hls::task t2(tree_traversal,fetchOut[1], rngStream[1 + id*3], traverseOut[1]);
    // hls_thread_local hls::task t3(tree_traversal,fetchOut[2], rngStream[2 + id*3], traverseOut[2]);
    //tree_traversal(fetchOut[0], rngStream[0+ id*3], traverseOut[0]);
    // tree_traversal(fetchOut[1], rngStream[1+ id*3], traverseOut[1]);
    // tree_traversal(fetchOut[2], rngStream[2+ id*3], traverseOut[2]);
    node_splitter(traverseOut, nodeSplitOut, blockIdx);
    page_splitter(nodeSplitOut, pageOut1, pageOut2);
    save( pageOut1, pageOut2, feedbackStream, pageBank1);
 
}

// void traverseBlocks(hls::stream_of_blocks<IPage> &fetchOut, hls::stream_of_blocks<IPage> &traverseOut){
//     #pragma HLS DATAFLOW
//     #pragma HLS INTERFACE port=return mode=ap_ctrl_none
//     tree_traversal(fetchOut, traverseOut);
// }
