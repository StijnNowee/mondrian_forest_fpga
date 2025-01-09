#include "top_lvl.hpp"
#include "train.hpp"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page pageBank1[MAX_PAGES],
    hls::stream<unit_interval, 20> &rngStream1,
    hls::stream<unit_interval, 20> &rngStream2
) {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES  max_write_burst_length=256 max_read_burst_length=256


    static hls::stream<FetchRequest> feedbackStream("FeedbackStream");

    hls::stream_of_blocks<IPage> fetchOutput;
    hls::stream_of_blocks<IPage> traverseOutput;
    hls::stream_of_blocks<IPage> splitterOut;

    pre_fetcher(inputFeatureStream, feedbackStream, fetchOutput, pageBank1);
    tree_traversal( fetchOutput, rngStream1, traverseOutput);
    splitter(traverseOutput, rngStream2, splitterOut);
    save(splitterOut, feedbackStream, pageBank1);
    

}