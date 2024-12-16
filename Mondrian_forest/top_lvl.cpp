#include "top_lvl.hpp"
#include "rng.hpp"

#include <cstdint>
#include <stdint.h>
#include "control_unit.hpp"
#include "train.hpp"
#include "hls_np_channel.h"

void top_lvl(
    feature_vector *inputFeature,
    Page pageBank1[MAX_PAGES]
) {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE mode=s_axilite port=inputFeature
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES  max_write_burst_length=256 max_read_burst_length=256


    hls::split::load_balance<unit_interval, 2, 20> rngStream;
    hls::stream<FetchRequest> fetchRequestStream("FetchRequestStream");
     hls::stream<FetchRequest> newFetchRequestStream("NewFetchRequestStream");
    hls::stream<FetchRequest> feedbackStream("FeedbackStream");

    FetchRequest feedbackRegister;
    
    generate_rng(rngStream.in);

    control_unit(inputFeature, newFetchRequestStream);
    streamMerger(newFetchRequestStream, feedbackRegister, fetchRequestStream);

    hls::stream_of_blocks<IPage> fetchOutput;
    hls::stream_of_blocks<IPage> traverseOutput;
    hls::stream_of_blocks<IPage> splitterOut;

    pre_fetcher(fetchRequestStream, fetchOutput, pageBank1);
    tree_traversal( fetchOutput, rngStream.out[0], traverseOutput);
    splitter(traverseOutput, rngStream.out[1], splitterOut);
    save(splitterOut, feedbackRegister, pageBank1);
    

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