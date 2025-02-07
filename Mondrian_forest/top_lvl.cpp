#include "top_lvl.hpp"
#include "train.hpp"
#include "rng.hpp"

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


    hls::stream_of_blocks<IPage,10> fetchOutput;
    hls::stream_of_blocks<IPage,3> traverseOutput;
    hls::stream_of_blocks<IPage,3> pageSplitterOut;
    hls::stream_of_blocks<IPage,3> nodeSplitterOut;

    hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK];

    #ifdef __SYNTHESIS__
        const int loopCount = size*TREES_PER_BANK;
    #else
        const int loopCount = TREES_PER_BANK;
    #endif

    #pragma HLS DATAFLOW
    hls::stream<unit_interval, 50> rngStream[2*BANK_COUNT];

    //hls::split::load_balance<unit_interval, 2, 50> rng_Stream;

    rng_generator(rngStream);

    feature_distributor(inputFeatureStream, splitFeatureStream, size);
    pre_fetcher(splitFeatureStream, feedbackStream, fetchOutput, pageBank1, loopCount, treeDoneStream);
    tree_traversal( fetchOutput, rngStream[0], traverseOutput, loopCount, treeDoneStream[0]);
    page_splitter(traverseOutput, pageSplitterOut, loopCount, treeDoneStream[1]);
    node_splitter(pageSplitterOut, rngStream[1], nodeSplitterOut, loopCount, treeDoneStream[2]);
    save(nodeSplitterOut, feedbackStream, pageBank1, loopCount, treeDoneStream[3]);
    

}