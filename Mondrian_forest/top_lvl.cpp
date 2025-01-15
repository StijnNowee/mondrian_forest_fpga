#include "top_lvl.hpp"
#include "train.hpp"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page *pageBank1,
    hls::stream<unit_interval, 20> &rngStream1,
    hls::stream<unit_interval, 20> &rngStream2
)  {
    #pragma HLS DATAFLOW
    
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES
    // #pragma HLS INTERFACE mode=s_axilite port=pageBank1
    // #pragma HLS INTERFACE s_axilite port=return

    static hls::stream<FetchRequest,5> feedbackStream("FeedbackStream");

    hls::stream<PageProperties> traverseControl ("TraverseControlStream");
    hls::stream<PageProperties> splitterControl ("SplitterControlStream");
    hls::stream<PageProperties> saveControl     ("SaveControlStream");

    hls::stream_of_blocks<Page,5> fetchOutput;
    hls::stream_of_blocks<Page,5> traverseOutput;
    hls::stream_of_blocks<Page,5> splitterOut;

   // Page fetchOutput, traverseOutput, splitterOut;
    // #pragma HLS stream type=pipo variable=fetchOutput depth=3
    // #pragma HLS stream type=pipo variable=traverseOutput depth=3
    // #pragma HLS stream type=pipo variable=splitterOut depth=3

    pre_fetcher(inputFeatureStream, feedbackStream, fetchOutput, traverseControl, pageBank1);
    tree_traversal( fetchOutput, rngStream1, traverseOutput, traverseControl, splitterControl);
    splitter(traverseOutput, rngStream2, splitterOut, splitterControl, saveControl);
    save(splitterOut, feedbackStream, saveControl, pageBank1);
    

}