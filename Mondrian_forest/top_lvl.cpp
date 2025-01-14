#include "top_lvl.hpp"
#include "train.hpp"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    volatile Page *pageBank1,
    hls::stream<unit_interval, 20> &rngStream1,
    hls::stream<unit_interval, 20> &rngStream2
)  {
    #pragma HLS DATAFLOW
    
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES
    // #pragma HLS INTERFACE mode=s_axilite port=pageBank1
    // #pragma HLS INTERFACE s_axilite port=return

    static hls::stream<FetchRequest,5> feedbackStream("FeedbackStream");

    hls::stream_of_blocks<IPage,5> fetchOutput;
    hls::stream_of_blocks<IPage,5> traverseOutput;
    hls::stream_of_blocks<IPage,5> splitterOut;

    pre_fetcher(inputFeatureStream, feedbackStream, fetchOutput, pageBank1);
    tree_traversal( fetchOutput, rngStream1, traverseOutput);
    splitter(traverseOutput, rngStream2, splitterOut);
    save(splitterOut, feedbackStream, pageBank1);
    

}