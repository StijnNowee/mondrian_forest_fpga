#include "train.hpp"
#include "hls_task.h"
#include "converters.hpp"

void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS], hls::stream<FetchRequest> &feedbackStream, Page *pageBank1, hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream, const int size)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE port=return mode=ap_ctrl_chain

    hls_thread_local hls::stream_of_blocks<IPage,3> fetchOutput[TRAVERSAL_BLOCKS];
    hls_thread_local hls::stream_of_blocks<IPage,3> traverseOutput[TRAVERSAL_BLOCKS];
    hls_thread_local hls::stream_of_blocks<IPage,3> pageSplitterOut;
    hls_thread_local hls::stream_of_blocks<IPage,3> nodeSplitterOut;

    pre_fetcher(fetchRequestStream, fetchOutput, pageBank1, smlTreeStream, treeUpdateCtrlStream);
    hls_thread_local hls::task trav[TRAVERSAL_BLOCKS];
    // for(int i = 0; i < TRAVERSAL_BLOCKS; i++){
    //    #pragma HLS UNROLL
    //     trav[i](tree_traversal, fetchOutput[i], rngStream[i], traverseOutput[i]);
    // }
    hls_thread_local hls::task t1(tree_traversal, fetchOutput[0], rngStream[0], traverseOutput[0]);
    hls_thread_local hls::task t2(tree_traversal, fetchOutput[1], rngStream[1], traverseOutput[1]);
    hls_thread_local hls::task t3(tree_traversal, fetchOutput[2], rngStream[2], traverseOutput[2]);
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
        #pragma HLS PIPELINE II=1
        out[n] = localPage[n];
    }
    out[MAX_NODES_PER_PAGE] = propertiesToRaw(p);
}
void read_page(IPage &localPage, PageProperties &p, hls::stream_of_blocks<IPage> &pageIn){
    #pragma HLS INLINE off
    hls::read_lock<IPage> in(pageIn);
    for(int n = 0; n < MAX_NODES_PER_PAGE + 1; n++){
        #pragma HLS PIPELINE II=1
        localPage[n] = in[n];
    }
    p = rawToProperties(in[MAX_NODES_PER_PAGE]);
}