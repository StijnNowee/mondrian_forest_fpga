#include "control_unit.hpp"
#include "train.hpp"
#include "hls_np_channel.h"

void control_unit(hls::stream<feature_vector> &inputFeatureStream,
    Page *pageBank1_read,
    Page *pageBank2_read,
    hls::split::load_balance<unit_interval, 4, 20> &rngStream)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE axis port=inputFeatureStream
    #pragma HLS INTERFACE axis port=rngStream

    hls::stream<FetchRequest, 10> train_stream[BANK_COUNT];
    //hls::stream<FetchRequest, 10> feedbackStream[BANK_COUNT];
    hls::split::round_robin<feature_vector, BANK_COUNT, 10> newFeatureStream;

    featureDistributer(inputFeatureStream, newFeatureStream.in);
    
    // for(int i = 0; i < BANK_COUNT; i++){
    //     //#pragma HLS DATAFLOW
    //     train(train_stream[i], rngStream.out[i*2], rngStream.out[i*2+1], feedbackStream[i], pageBank1);
    //     streamMerger(feedbackStream[i], newFeatureStream.out[i], train_stream[i]);
    // }
    streamMerger(newFeatureStream.out[0], train_stream[0]);
    train(train_stream[0], rngStream.out[0], rngStream.out[1], pageBank1_read);
    streamMerger(newFeatureStream.out[1], train_stream[1]);
    train(train_stream[1], rngStream.out[2], rngStream.out[3], pageBank2_read);
    


}

void streamMerger(hls::stream<feature_vector> &newFeatureStream, hls::stream<FetchRequest> &requestStream)
{

    if (!newFeatureStream.empty()){
        requestStream.write(FetchRequest{.feature = newFeatureStream.read(), .pageIdx = 0});
    }
} 

void featureDistributer(hls::stream<feature_vector> &inputFeatureStream, hls::stream<feature_vector> &newFeatureStream)
{
    feature_vector input = inputFeatureStream.read();
    newFeatureStream.write(input);
    newFeatureStream.write(input);
}