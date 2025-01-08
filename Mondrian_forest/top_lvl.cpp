#include "top_lvl.hpp"
#include "train.hpp"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page pageBank1[MAX_PAGES],
    hls::split::load_balance<unit_interval, 2, 20> &rngStream
) {
    #pragma HLS DATAFLOW
    //#pragma HLS INTERFACE mode=s_axilite port=inputFeature
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES  max_write_burst_length=256 max_read_burst_length=256


    // hls::split::load_balance<unit_interval, 2, 20> rngStream;
    //hls::stream<feature_vector> fetchRequestStream("FetchRequestStream");
    static hls::stream<FetchRequest> feedbackStream("FeedbackStream");

    //static FetchRequest feedbackRegister = FetchRequest();
    //#pragma HLS ARRAY_PARTITION variable=feedbackRegister dim=1 type=complete
    
    //generate_rng(rngStream.in);

    //control_unit(inputFeature, fetchRequestStream);
    //streamMerger(newFetchRequestStream, feedbackRegister, fetchRequestStream);

    hls::stream_of_blocks<IPage> fetchOutput;
    hls::stream_of_blocks<IPage> traverseOutput;
    hls::stream_of_blocks<IPage> splitterOut;

    pre_fetcher(inputFeatureStream, feedbackStream, fetchOutput, pageBank1);
    tree_traversal( fetchOutput, rngStream.out[0], traverseOutput);
    splitter(traverseOutput, rngStream.out[1], splitterOut);
    save(splitterOut, feedbackStream, pageBank1);
    

}

void streamMerger(hls::stream<FetchRequest> &newRequest, FetchRequest &feedbackRegister, hls::stream<FetchRequest> &fetchRequest)
{
    while(true){
        if(feedbackRegister.valid){
            fetchRequest.write(feedbackRegister);
            feedbackRegister.valid =false;
        }else if (!newRequest.empty()) {
            fetchRequest.write(newRequest.read());
        }
    }
}