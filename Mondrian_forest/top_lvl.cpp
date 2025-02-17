#include "top_lvl.hpp"
#include "train.hpp"
#include "rng.hpp"
#include "hls_task.h"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page *pageBank1
)  {
    #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    #pragma HLS INTERFACE axis port=inputFeatureStream depth=10
    #pragma HLS DATAFLOW
    #pragma HLS stable variable=pageBank1
    

    // #pragma HLS INTERFACE mode=s_axilite port=pageBank1
    // #pragma HLS INTERFACE s_axilite port=return

    hls_thread_local hls::stream<FetchRequest,5> feedbackStream("FeedbackStream");


    hls_thread_local hls::stream_of_blocks<IPage,10> fetchOutput;
    hls_thread_local hls::stream_of_blocks<IPage,3> traverseOutput;
    hls_thread_local hls::stream_of_blocks<IPage,3> pageSplitterOut;
    hls_thread_local hls::stream_of_blocks<IPage,3> nodeSplitterOut;

    hls_thread_local hls::stream<input_vector,5> splitFeatureStream[TREES_PER_BANK];

    // #ifdef __SYNTHESIS__
    //     const int loopCount = size*TREES_PER_BANK;
    // #else
    //     const int loopCount = TREES_PER_BANK;
    // #endif

    
    hls_thread_local hls::stream<unit_interval, 100> rngStream[2*BANK_COUNT];

    //hls::split::load_balance<unit_interval, 2, 50> rng_Stream;

    hls_thread_local hls::task t1(rng_generator, rngStream);

    //hls_thread_local hls::task t2(feature_distributor, inputFeatureStream, splitFeatureStream);
    hls_thread_local hls::task t3(pre_fetcher, inputFeatureStream, feedbackStream, fetchOutput, pageBank1);
    hls_thread_local hls::task t4(tree_traversal, fetchOutput, rngStream[0], traverseOutput);
    hls_thread_local hls::task t5(page_splitter, traverseOutput, pageSplitterOut);
    hls_thread_local hls::task t6(node_splitter, pageSplitterOut, rngStream[1], nodeSplitterOut);
    hls_thread_local hls::task t7(save, nodeSplitterOut, feedbackStream, pageBank1);
    
}