#include "train.hpp"
#include <hls_np_channel.h>
#include "hls_task.h"
#include "rng.hpp"

void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<FetchRequest> &feedbackStream, Page *pageBank1, trees_t &smlTreeBank, const int size)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE port=return mode=ap_ctrl_chain

    hls_thread_local hls::stream_of_blocks<IPage,10> fetchOutput;
    hls_thread_local hls::stream_of_blocks<IPage,3> traverseOutput;
    hls_thread_local hls::stream_of_blocks<IPage,3> pageSplitterOut;
    hls_thread_local hls::stream_of_blocks<IPage,3> nodeSplitterOut;
    
    hls_thread_local hls::split::load_balance<unit_interval, 2> rngStream;

    //hls_thread_local hls::task t2(feature_distributor, inputFeatureStream, splitFeatureStream);
    pre_fetcher(fetchRequestStream, fetchOutput, pageBank1, smlTreeBank);
    //hls_thread_local hls::task t1(rng_generator, rngStream.in);
    hls_thread_local hls::task t3(tree_traversal, fetchOutput, rngStream.out[0], traverseOutput);
    hls_thread_local hls::task t4(page_splitter,traverseOutput, pageSplitterOut);
    hls_thread_local hls::task t5(node_splitter,pageSplitterOut, rngStream.out[1], nodeSplitterOut);
    save( nodeSplitterOut, feedbackStream, pageBank1, size);
 
}



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

void write_page(const IPage &localPage, const PageProperties &p, hls::stream_of_blocks<IPage> &pageOut){
    hls::write_lock<IPage> out(pageOut);
    for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        out[n] = localPage[n];
    }
    convertPropertiesToRaw(p, out[MAX_NODES_PER_PAGE]);
}
void read_page(IPage &localPage, PageProperties &p, hls::stream_of_blocks<IPage> &pageIn){
    hls::read_lock<IPage> in(pageIn);
    for(int n = 0; n < MAX_NODES_PER_PAGE + 1; n++){
        localPage[n] = in[n];
    }
    convertRawToProperties(in[MAX_NODES_PER_PAGE], p);
}