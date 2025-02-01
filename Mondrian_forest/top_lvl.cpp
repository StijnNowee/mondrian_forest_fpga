#include "top_lvl.hpp"
#include "train.hpp"
#include "rng.hpp"
#include "hls_np_channel.h"
#include "hls_task.h"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page *pageBank1,
    const int size
)  {
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    #pragma HLS INTERFACE axis port=inputFeatureStream depth=10
    

    // #pragma HLS INTERFACE mode=s_axilite port=pageBank1
    // #pragma HLS INTERFACE s_axilite port=return

    static hls::stream<FetchRequest,5> feedbackStream("FeedbackStream");
    
    hls::stream<bool,5> treeDoneStream[4];


    hls::stream_of_blocks<IPage,5> fetchOutput;
    hls::stream_of_blocks<IPage,5> traverseOutput;
    hls::stream_of_blocks<IPage,5> pageSplitterOut;
    hls::stream_of_blocks<IPage,5> nodeSplitterOut;

    hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK];
    


    const int loopCount = size*TREES_PER_BANK;

    #pragma HLS DATAFLOW
    hls::split::load_balance<unit_interval, 2, 50> rng_Stream;
    hls_thread_local hls::task gen(rng_generator, rng_Stream.in);
    //rng_generator(rng_Stream.in);
    feature_distributor(inputFeatureStream, splitFeatureStream, size);
    pre_fetcher(splitFeatureStream, feedbackStream, fetchOutput, pageBank1, loopCount, treeDoneStream);
    tree_traversal( fetchOutput, rng_Stream.out[0], traverseOutput, loopCount, treeDoneStream[0]);
    page_splitter(traverseOutput, pageSplitterOut, loopCount, treeDoneStream[1]);
    node_splitter(pageSplitterOut, rng_Stream.out[1], nodeSplitterOut, loopCount, treeDoneStream[2]);
    save(nodeSplitterOut, feedbackStream, pageBank1, loopCount, treeDoneStream[3]);
    

}