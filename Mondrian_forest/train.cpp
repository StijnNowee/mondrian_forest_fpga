#include "train.hpp"
#include "converters.hpp"

void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS], hls::stream<FetchRequest> &feedbackStream, Page *pageBank1, const int &id)
{
    #pragma HLS DATAFLOW

    // hls::stream_of_blocks<IPage,2> fetchOutput[TRAVERSAL_BLOCKS];
    // hls::stream_of_blocks<IPage,2> traverseOutput[TRAVERSAL_BLOCKS];
    // hls::stream_of_blocks<IPage,3> pageSplitterOut;
    // hls::stream_of_blocks<IPage,3> nodeSplitterOut;
    IPage fetchOut;
    IPage traverseOut;
    IPage pageOut1, pageOut2;
    IPage nodeSplitOut1, nodeSplitOut2;


    pre_fetcher(fetchRequestStream, fetchOut, pageBank1);
    //hls_thread_local hls::task trav[TRAVERSAL_BLOCKS];
    // for(int i = 0; i < TRAVERSAL_BLOCKS; i++){
    //    #pragma HLS UNROLL
    //     trav[i](tree_traversal, fetchOutput[i], rngStream[i], traverseOutput[i]);
    // }
    tree_traversal(fetchOut, rngStream[0 + id*3], traverseOut);
    // tree_traversal(fetchOutput[1], rngStream[1+ id*3], traverseOutput[1]);
    // tree_traversal(fetchOutput[2], rngStream[2+ id*3], traverseOutput[2]);
    page_splitter(traverseOut, pageOut1, pageOut2);
    node_splitter(pageOut1, pageOut2, nodeSplitOut1, nodeSplitOut2);
    save( nodeSplitOut1, nodeSplitOut2, feedbackStream, pageBank1);
 
}

void write_page(const IPage &localPage, const PageProperties &p, hls::stream_of_blocks<IPage> &pageOut){
    #pragma HLS inline off
    hls::write_lock<IPage> out(pageOut);
    for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        #pragma HLS PIPELINE II=1
        out[n] = localPage[n];
    }
    out[MAX_NODES_PER_PAGE] = propertiesToRaw(p);
}
void read_page(IPage &localPage, PageProperties &p, hls::stream_of_blocks<IPage> &pageIn){
    #pragma HLS inline off
    hls::read_lock<IPage> in(pageIn);
    for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        #pragma HLS PIPELINE II=1
        localPage[n] = in[n];
    }
    p = rawToProperties(in[MAX_NODES_PER_PAGE]);
}