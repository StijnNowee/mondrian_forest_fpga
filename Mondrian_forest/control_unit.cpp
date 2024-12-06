#include "control_unit.hpp"
#include "train.hpp"
#include "hls_np_channel.h"

void control_unit(hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Page *pageBank1_read,
    Page *pageBank1_write,
    Page *pageBank2_read,
    Page *pageBank2_write,
    hls::split::load_balance<unit_interval, 4, 20> &rngStream)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE axis port=inputFeatureStream
    #pragma HLS INTERFACE axis port=inputLabelStream
    #pragma HLS INTERFACE axis port=rngStream
    #pragma HLS INTERFACE m_axi port=pageBank1_read
    #pragma HLS INTERFACE m_axi port=pageBank1_write
    #pragma HLS INTERFACE m_axi port=pageBank2_read
    #pragma HLS INTERFACE m_axi port=pageBank2_write

    hls::stream<FetchRequest, 10> train_stream[BANK_COUNT];
    hls::stream<FetchRequest, 10> feedbackStream[BANK_COUNT];
    hls::split::round_robin<feature_vector, BANK_COUNT, 10> newFeatureStream;

    featureDistributer(inputFeatureStream, newFeatureStream.in);
    
    // for(int i = 0; i < BANK_COUNT; i++){
    //     //#pragma HLS DATAFLOW
    //     train(train_stream[i], rngStream.out[i*2], rngStream.out[i*2+1], feedbackStream[i], pageBank1);
    //     streamMerger(feedbackStream[i], newFeatureStream.out[i], train_stream[i]);
    // }
    streamMerger(feedbackStream[0], newFeatureStream.out[0], train_stream[0]);
    train(train_stream[0], rngStream.out[0], rngStream.out[1], feedbackStream[0], pageBank1_read, pageBank1_write);
    streamMerger(feedbackStream[1], newFeatureStream.out[1], train_stream[1]);
    train(train_stream[1], rngStream.out[2], rngStream.out[3], feedbackStream[1], pageBank2_read, pageBank2_write);
    


}

void streamMerger(hls::stream<FetchRequest> &feedbackStream, hls::stream<feature_vector> &newFeatureStream, hls::stream<FetchRequest> &requestStream)
{
    if(!feedbackStream.empty()){
        requestStream.write(feedbackStream.read());
    }else if (!newFeatureStream.empty()){
        requestStream.write(FetchRequest{.feature = newFeatureStream.read(), .pageIdx = 0});
    }
} 

void featureDistributer(hls::stream<feature_vector> &inputFeatureStream, hls::stream<feature_vector> &newFeatureStream)
{
    feature_vector input = inputFeatureStream.read();
    newFeatureStream.write(input);
    newFeatureStream.write(input);
}