#include "processing_unit.hpp"
#include "train.hpp"
#include "inference.hpp"
#include "converters.hpp"
#include <hls_task.h>
#include <iostream>

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<input_vector> &inferenceInputStream, const int size);
void control_unit(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], const int size, Page *pageBank, const int &id, hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS]);
void send_new_request(hls::stream<input_vector> &splitFeatureStream, hls::stream<FetchRequest> &fetchRequestStream, const int treeID, const int freePageIndex);
void process_feedback(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, int freePageIndex[TREES_PER_BANK], int &samplesProcessed, ap_uint<TREES_PER_BANK> &processing);
void processing_unit(hls::stream<input_t> &inputFeatureStream, hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS], Page *pageBank, const InputSizes &sizes, hls::stream<ClassDistribution> &inferenceOutputStream, const int &id)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE port=return mode=ap_ctrl_chain

    //trees_t smlTreeBank;
    //hls::stream_of_blocks<trees_t, 2> smlTreeStream;
    //#pragma HLS ARRAY_PARTITION variable=smlTreeStream dim=1 type=complete
   // hls::stream<bool,1> treeUpdateCtrlStream;
    //#pragma HLS ARRAY_PARTITION variable=smlTreeBank dim=1 type=complete
    //#pragma HLS BIND_STORAGE variable=smlTreeStream type=ram_1p impl=uram
    
    
    
    hls::stream<input_vector> inferenceInputStream("inferenceInputStream");
    hls::stream<input_vector,3> splitFeatureStream[TREES_PER_BANK];
    feature_distributor(inputFeatureStream, splitFeatureStream, inferenceInputStream, sizes.total);
    control_unit(splitFeatureStream, sizes.training, pageBank, id, rngStream);
    
    
    //train(fetchRequestStream, rngStream, feedbackStream, pageBank, smlTreeStream, treeUpdateCtrlStream,sizes.training, id);
    //inference(inferenceInputStream, inferenceOutputStream, smlTreeStream, treeUpdateCtrlStream, sizes.inference);
   
}

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<input_vector> &inferenceInputStream, const int size)
{
    for(int i = 0; i < size; i++){
        
        auto rawInput = newFeatureStream.read();
        
        input_vector newInput = convertInputToVector(rawInput);
        if(newInput.inferenceSample){
            inferenceInputStream.write(newInput);
        }else{
            for(int t = 0; t < TREES_PER_BANK; t++){
                splitFeatureStream[t].write(newInput);
            }
        }
    }
}

void control_unit(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], const int size, Page *pageBank, const int &id, hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS])
{
    int freePageIndex[TREES_PER_BANK];
    // TreeStatus status[TREES_PER_BANK];
    ap_uint<TREES_PER_BANK> processing;
    int traverseBlockID = 0;
    for(int t = 0; t < TREES_PER_BANK; t++){
        freePageIndex[t] = 1;
        processing[t] = false;
    }
    hls::stream<FetchRequest,TREES_PER_BANK> feedbackStream("FeedbackStream");
    hls::stream<FetchRequest,TREES_PER_BANK> fetchRequestStream("FetchRequestStream");
    for(int i = 0; i < size*TREES_PER_BANK;){
        process_feedback(splitFeatureStream, feedbackStream, fetchRequestStream, freePageIndex, i, processing);
        traverseBlockID = (++traverseBlockID == TRAVERSAL_BLOCKS) ? 0 : traverseBlockID;
        train(fetchRequestStream, rngStream, feedbackStream, pageBank, id, traverseBlockID);
    }
}

void process_feedback(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, int freePageIndex[TREES_PER_BANK], int &samplesProcessed, ap_uint<TREES_PER_BANK> &processing)
{
    if(!feedbackStream.empty()){
        FetchRequest request = feedbackStream.read();
        if(request.extraPage){
            freePageIndex[request.treeID]++;
        }
        
        if(request.needNewPage){
            fetchRequestStream.write(request);
        }else{
            samplesProcessed++;
            processing[request.treeID] = false;
        }
    }
    for(int t = 0; t < TREES_PER_BANK; t++){
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

void send_new_request(hls::stream<input_vector> &splitFeatureStream, hls::stream<FetchRequest> &fetchRequestStream, const int treeID, const int freePageIndex)
{
    FetchRequest newRequest{splitFeatureStream.read(), 0, treeID, freePageIndex, false, false, false, false};
    fetchRequestStream.write(newRequest);
}