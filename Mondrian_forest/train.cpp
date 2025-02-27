#include "train.hpp"
#include "top_lvl.hpp"
#include <hls_np_channel.h>
#include "hls_task.h"
#include "rng.hpp"
void train(hls::stream<input_t> &inputFeatureStream, hls::stream<node_t> &outputStream, hls::stream<bool> &controlOutputStream,Page *pageBank1, hls::stream_of_blocks<trees_t> &treeStream)
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
    hls_thread_local hls::task t7(pre_fetcher, splitFeatureStream, feedbackStream, fetchOutput, pageBank1, treeStream);
    hls_thread_local hls::task t3(tree_traversal, fetchOutput, rngStream.out[0], traverseOutput);
    hls_thread_local hls::task t4(page_splitter,traverseOutput, pageSplitterOut);
    hls_thread_local hls::task t5(node_splitter,pageSplitterOut, rngStream.out[1], nodeSplitterOut);
    hls_thread_local hls::task t6(save, nodeSplitterOut, feedbackStream, outputStream, controlOutputStream, pageBank1);
}

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK])
{
    auto rawInput = newFeatureStream.read();
    input_vector newInput;
    convertInputToVector(rawInput, newInput);
    for(int t = 0; t < TREES_PER_BANK; t++){
        splitFeatureStream[t].write(newInput);
    }
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