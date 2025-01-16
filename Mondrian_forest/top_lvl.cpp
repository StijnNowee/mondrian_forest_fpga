#include "top_lvl.hpp"
#include "train.hpp"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page *pageBank1,
    hls::stream<unit_interval, 20> &rngStream1,
    hls::stream<unit_interval, 20> &rngStream2,
    int size
)  {
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES
    // #pragma HLS INTERFACE mode=s_axilite port=pageBank1
    // #pragma HLS INTERFACE s_axilite port=return

    static hls::stream<FetchRequest,5> feedbackStream("FeedbackStream");

    // hls::stream<PageProperties, 5> traverseControl ("TraverseControlStream");
    // hls::stream<PageProperties, 5> splitterControl ("SplitterControlStream");
    // hls::stream<PageProperties, 5> saveControl     ("SaveControlStream");

    hls::stream_of_blocks<IPage,5> fetchOutput;
    hls::stream_of_blocks<IPage,5> traverseOutput;
    hls::stream_of_blocks<IPage,5> splitterOut;

    #pragma HLS DATAFLOW
    pre_fetcher(inputFeatureStream, feedbackStream, fetchOutput, pageBank1, size);
    tree_traversal( fetchOutput, rngStream1, traverseOutput, size);
    splitter(traverseOutput, rngStream2, splitterOut, size);
    save(splitterOut, feedbackStream, pageBank1, size);
    

}