#include "control_unit.hpp"
#include "train.hpp"
#include "hls_np_channel.h"

void control_unit(feature_vector *newFeature,
    Page *pageBank1_read,
    Page *pageBank2_read,
    hls::split::load_balance<unit_interval, 4, 20> &rngStream)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE mode=s_axilite port=newFeature
    #pragma HLS INTERFACE axis port=rngStream

    hls::stream<FetchRequest, 10> train_stream[BANK_COUNT];
    // hls::stream<FetchRequest, 10> feedbackStream[BANK_COUNT];

    
    streamMerger(newFeatureStream.out[0], train_stream[0]);//, feedbackStream[0]);
    train(train_stream[0], rngStream.out[0], rngStream.out[1], pageBank1_read); //, feedbackStream[0]

    // streamMerger(newFeatureStream.out[1], train_stream[1]);//, feedbackStream[1]);
    // train(train_stream[1], rngStream.out[2], rngStream.out[3], pageBank2_read); //, feedbackStream[1]
    


}

void streamMerger(hls::stream<feature_vector> &newFeatureStream, hls::stream<FetchRequest> &requestStream)//, hls::stream<FetchRequest> &feedbackStream)
{
    // if (!feedbackStream.empty()){
    //     requestStream.write(feedbackStream.read());
    // }else 
    if (!newFeatureStream.empty()){
        std::cout << "newFeature size: " << newFeatureStream.size() << std::endl;
        requestStream.write(FetchRequest{.feature = newFeatureStream.read(), .pageIdx = 0});
    }
} 