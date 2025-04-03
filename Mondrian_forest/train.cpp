#include "train.hpp"
#include "converters.hpp"
#include <hls_task.h>
void traverseBlocks(hls::stream_of_blocks<IPage> &fetchOut, hls::stream_of_blocks<IPage> &traverseOut);
void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS], hls::stream<FetchRequest> &feedbackStream, Page *pageBank1)
{
    #pragma HLS DATAFLOW
    hls::stream_of_blocks<IPage, 3> fetchOut[TRAVERSAL_BLOCKS], traverseOut[TRAVERSAL_BLOCKS];
    hls::stream_of_blocks<IPage, 3> pageOut1, pageOut2, nodeSplitOut1, nodeSplitOut2;


    pre_fetcher(fetchRequestStream, fetchOut, pageBank1);
    //hls_thread_local hls::task trav[TRAVERSAL_BLOCKS];
    for(int i = 0; i < TRAVERSAL_BLOCKS; i++){
       #pragma HLS UNROLL
        tree_traversal(fetchOut[i], rngStream[i], traverseOut[i]);
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
    page_splitter(traverseOut, pageOut1, pageOut2);
    node_splitter(pageOut1, pageOut2, nodeSplitOut1, nodeSplitOut2);
    save( nodeSplitOut1, nodeSplitOut2, feedbackStream, pageBank1);
 
}

// void traverseBlocks(hls::stream_of_blocks<IPage> &fetchOut, hls::stream_of_blocks<IPage> &traverseOut){
//     #pragma HLS DATAFLOW
//     #pragma HLS INTERFACE port=return mode=ap_ctrl_none
//     tree_traversal(fetchOut, traverseOut);
// }
