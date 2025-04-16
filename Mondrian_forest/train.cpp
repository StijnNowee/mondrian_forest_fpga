#include "train.hpp"
#include "processing_unit.hpp"
void rng_splitter(hls::stream<unit_interval> &rngIn, hls::stream<unit_interval> &rngOut);
void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], hls::stream<Feedback> &feedbackStream, Page *pageBank, const int &blockIdx)
{
    #pragma HLS DATAFLOW disable_start_propagation
    hls::stream_of_blocks<IPage, 3> fetchOut[TRAIN_TRAVERSAL_BLOCKS], traverseOut[TRAIN_TRAVERSAL_BLOCKS], nodeSplitOut, pageOut1, pageOut2;
    
    fetcher<TRAIN_TRAVERSAL_BLOCKS, FetchRequest, PageProperties>(fetchRequestStream, fetchOut, pageBank, blockIdx);
    for(int i = 0; i < TRAIN_TRAVERSAL_BLOCKS; i++){
       #pragma HLS UNROLL
        tree_traversal(fetchOut[i], rngStream[i], traverseOut[i]);
    }
    
    node_splitter(traverseOut, nodeSplitOut, blockIdx);
    page_splitter(nodeSplitOut, pageOut1, pageOut2);
    save( pageOut1, pageOut2, feedbackStream, pageBank);
 
}