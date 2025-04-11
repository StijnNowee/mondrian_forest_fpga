#include "processing_unit.hpp"
#include "train.hpp"
#include "converters.hpp"
#include <iostream>
#include <hls_math.h>
#include "inference.hpp"

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<input_vector> inferenceInputStream[TREES_PER_BANK], const int size);
void train_control_unit(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], const int &size, Page *pageBank, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS]);
void inference_control_unit(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<ClassDistribution> &voterOutputStream, const int &size, const Page *pageBank);
void send_new_request(hls::stream<input_vector> &splitFeatureStream, hls::stream<FetchRequest> &fetchRequestStream, const int &treeID, const int &freePageIndex);
void process_feedback(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<Feedback> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, int freePageIndex[TREES_PER_BANK], int &samplesProcessed, ap_uint<TREES_PER_BANK> &processing);
void process_inference_feedback(hls::stream<IFeedback> &feedbackStream, hls::stream<IFetchRequest> &fetchRequestStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], int &samplesProcessed, ap_uint<TREES_PER_BANK> &processing);

void processing_unit(hls::stream<input_t> &inputFeatureStream, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], Page *pageBank, const Page *readOnlyPageBank, const InputSizes &sizes, hls::stream<ClassDistribution> &inferenceOutputStream)
{
    #pragma HLS inline
    hls::stream<input_vector,3> splitFeatureStream[TREES_PER_BANK], inferenceInputStream[TREES_PER_BANK];
    feature_distributor(inputFeatureStream, splitFeatureStream, inferenceInputStream, sizes.total);

    train_control_unit(splitFeatureStream, sizes.training, pageBank, rngStream);
    inference_control_unit(inferenceInputStream, inferenceOutputStream, sizes.inference, readOnlyPageBank);
   
}

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<input_vector> inferenceInputStream[TREES_PER_BANK], const int size)
{
    for(int i = 0; i < size; i++){
        
        auto rawInput = newFeatureStream.read();
        
        input_vector newInput = convertInputToVector(rawInput);
        if(newInput.inferenceSample){
            for(int t = 0; t < TREES_PER_BANK; t++){
                inferenceInputStream[t].write(newInput);
            }
        }else{
            for(int t = 0; t < TREES_PER_BANK; t++){
                splitFeatureStream[t].write(newInput);
            }
        }
    }
}

void train_control_unit(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], const int &size, Page *pageBank, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS])
{
    int freePageIndex[TREES_PER_BANK];
    ap_uint<TREES_PER_BANK> processing;
    int traverseBlockID = 0;
    for(int t = 0; t < TREES_PER_BANK; t++){
        freePageIndex[t] = 1;
        processing[t] = false;
    }
    hls::stream<Feedback,TREES_PER_BANK> feedbackStream("Train feedbackStream");
    hls::stream<FetchRequest,TREES_PER_BANK> fetchRequestStream("Train fetchRequestStream");
    for(int i = 0; i < size*TREES_PER_BANK;){
        process_feedback(splitFeatureStream, feedbackStream, fetchRequestStream, freePageIndex, i, processing);
        traverseBlockID = (++traverseBlockID == TRAIN_TRAVERSAL_BLOCKS) ? 0 : traverseBlockID;
        train(fetchRequestStream, rngStream, feedbackStream, pageBank, traverseBlockID);
    }
}

void inference_control_unit(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<ClassDistribution> &voterOutputStream, const int &size, const Page *pageBank)
{
    ap_uint<TREES_PER_BANK> processing;
    int traverseBlockID = 0;
    for(int t = 0; t < TREES_PER_BANK; t++){
        processing[t] = false;
    }
    hls::stream<IFeedback,TREES_PER_BANK> feedbackStream("Inference feedbackStream");
    hls::stream<IFetchRequest,TREES_PER_BANK> fetchRequestStream("Inference fetchRequestStream");
    for(int i = 0; i < size*TREES_PER_BANK;){
        process_inference_feedback(feedbackStream, fetchRequestStream, splitFeatureStream, i, processing);
        traverseBlockID = (++traverseBlockID == INF_TRAVERSAL_BLOCKS) ? 0 : traverseBlockID;
        inference(fetchRequestStream, feedbackStream, voterOutputStream, pageBank, traverseBlockID);
    }
}

void process_feedback(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<Feedback> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, int freePageIndex[TREES_PER_BANK], int &samplesProcessed, ap_uint<TREES_PER_BANK> &processing)
{
    if(!feedbackStream.empty()){
        Feedback feedback = feedbackStream.read();
        if(feedback.extraPage){
            freePageIndex[feedback.treeID]++;
        }
        
        if(feedback.needNewPage){
            FetchRequest newRequest(feedback);
            fetchRequestStream.write(newRequest);
        }else{
            samplesProcessed++;
            processing[feedback.treeID] = false;
        }
    }
    for(int t = 0; t < TREES_PER_BANK; t++){
        #pragma HLS PIPELINE II=2
        if(processing[t] == false && !splitFeatureStream[t].empty() && !fetchRequestStream.full()
        #ifndef __SYNTHESIS__
        && fetchRequestStream.empty()
        #endif
        ){
            processing[t] = true;
            send_new_request(splitFeatureStream[t], fetchRequestStream, t, freePageIndex[t]);
        }
    }
}

void process_inference_feedback(hls::stream<IFeedback> &feedbackStream, hls::stream<IFetchRequest> &fetchRequestStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], int &samplesProcessed, ap_uint<TREES_PER_BANK> &processing)
{
    while(!feedbackStream.empty()){
        IFeedback feedback = feedbackStream.read();
        if(feedback.needNewPage){
            IFetchRequest newRequest(feedback);
            fetchRequestStream.write(newRequest);
        }else{
            samplesProcessed++;
            processing[feedback.treeID] = false;
        }
    }
    for(int t = 0; t < TREES_PER_BANK; t++){
        #pragma HLS PIPELINE II=2
        if(processing[t] == false && !splitFeatureStream[t].empty() && !fetchRequestStream.full()
        #ifndef __SYNTHESIS__
        && fetchRequestStream.empty()
        #endif
        ){
            processing[t] = true;
            IFetchRequest newRequest(splitFeatureStream[t].read(), 0, t);
            fetchRequestStream.write(newRequest);
        }
    }
}

void send_new_request(hls::stream<input_vector> &splitFeatureStream, hls::stream<FetchRequest> &fetchRequestStream, const int &treeID, const int &freePageIndex)
{
    FetchRequest newRequest(splitFeatureStream.read(), 0, treeID, freePageIndex);
    fetchRequestStream.write(newRequest);
}

void update_weight(Node_hbm &node)
{
    int tmpTotalCounts = 0;
    const bool leaf = node.leaf();
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        tmpTotalCounts += (leaf) ? node.counts[c] : node.getTotalTabs(c);
    }
    ap_ufixed<9,1> tmpDivision = ap_ufixed<9,1>(1.0) / (tmpTotalCounts + ap_ufixed<8,7>(BETA));
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        node.weight[c] = (((leaf) ? node.counts[c] : node.getTotalTabs(c)) + unit_interval(ALPHA)) *tmpDivision;
    }
}