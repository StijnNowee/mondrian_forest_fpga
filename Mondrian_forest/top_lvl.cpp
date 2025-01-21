#include "top_lvl.hpp"
#include "train.hpp"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page *pageBank1,
    hls::stream<unit_interval, 100> &rngStream1,
    hls::stream<unit_interval, 100> &rngStream2,
    const int size
)  {
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    #pragma HLS INTERFACE axis port=rngStream1 depth=100
    #pragma HLS INTERFACE axis port=rngStream2 depth=100
    #pragma HLS INTERFACE axis port=inputFeatureStream depth=10
    // #pragma HLS INTERFACE mode=s_axilite port=pageBank1
    // #pragma HLS INTERFACE s_axilite port=return

    static hls::stream<FetchRequest,5> feedbackStream("FeedbackStream");


    hls::stream_of_blocks<IPage,5> fetchOutput;
    hls::stream_of_blocks<IPage,5> traverseOutput;
    hls::stream_of_blocks<IPage,5> splitterOut;

    hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK];

    #if(defined __SYNTHESIS__)
        const int loopCount = size*TREES_PER_BANK;
    #else
        const int loopCount = TREES_PER_BANK;
    #endif

    #pragma HLS DATAFLOW
    feature_distributor(inputFeatureStream, splitFeatureStream, size);
    pre_fetcher(splitFeatureStream, feedbackStream, fetchOutput, pageBank1, loopCount);
    tree_traversal( fetchOutput, rngStream1, traverseOutput, loopCount);
    splitter(traverseOutput, rngStream2, splitterOut, loopCount);
    save(splitterOut, feedbackStream, pageBank1, loopCount);
    

}