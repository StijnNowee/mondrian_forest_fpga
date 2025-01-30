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
    hls::stream<bool,5> treeDoneStream[4];


    hls::stream_of_blocks<IPage,5> fetchOutput;
    hls::stream_of_blocks<IPage,5> traverseOutput;
    hls::stream_of_blocks<IPage,5> pageSplitterOut;
    hls::stream_of_blocks<IPage,5> nodeSplitterOut;

    hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK];

    #if(defined __SYNTHESIS__)
        const int loopCount = size*TREES_PER_BANK;
    #else
        const int loopCount = size*TREES_PER_BANK;
    #endif

    #pragma HLS DATAFLOW
    feature_distributor(inputFeatureStream, splitFeatureStream, size);
    pre_fetcher(splitFeatureStream, feedbackStream, fetchOutput, pageBank1, loopCount, treeDoneStream);
    tree_traversal( fetchOutput, rngStream1, traverseOutput, loopCount, treeDoneStream[0]);
    page_splitter(traverseOutput, pageSplitterOut, loopCount, treeDoneStream[1]);
    node_splitter(pageSplitterOut, rngStream2, nodeSplitterOut, loopCount, treeDoneStream[2]);
    save(nodeSplitterOut, feedbackStream, pageBank1, loopCount, treeDoneStream[3]);
    

}