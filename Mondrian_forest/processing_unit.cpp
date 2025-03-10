#include "processing_unit.hpp"
#include "train.hpp"
#include "inference.hpp"
#include "top_lvl.hpp"
#include <hls_task.h>

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], const int size);
void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, const int size);

void processing_unit(hls::stream<input_t> &inputFeatureStream,Page *pageBank1, const int size, hls::stream<input_t> &inferenceInputStream, hls::stream<Result> &inferenceOutputStream)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE port=return mode=ap_ctrl_chain
    
    hls_thread_local hls::stream<FetchRequest,5> feedbackStream("FeedbackStream");
    hls_thread_local hls::stream<FetchRequest,5> fetchRequestStream("FetchRequestStream");
    hls_thread_local hls::stream<input_vector,3> splitFeatureStream[TREES_PER_BANK];
    feature_distributor(inputFeatureStream, splitFeatureStream, size);
    tree_controller(splitFeatureStream, feedbackStream, fetchRequestStream, size);
    
    hls_thread_local hls::stream_of_blocks<trees_t, 3> treeStream;
    hls_thread_local hls::stream<bool> treeUpdateCtrlStream("TreeUpdateCtrlStream");

    train(fetchRequestStream, feedbackStream, treeStream, treeUpdateCtrlStream, pageBank1, size);
    inference(inferenceInputStream, inferenceOutputStream, treeStream, treeUpdateCtrlStream);
}

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], const int size)
{
    for(int i = 0; i < size; i++){
        auto rawInput = newFeatureStream.read();
        input_vector newInput;
        convertInputToVector(rawInput, newInput);
        for(int t = 0; t < TREES_PER_BANK; t++){
            std::cout << "writeFeature" << std::endl;
            splitFeatureStream[t].write(newInput);
        }
    }
}

void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, const int size)
{
    TreeStatus status[TREES_PER_BANK];
    //int processCounter[TREES_PER_BANK];
    int processCounter = 0;

    for(int t = 0; t < TREES_PER_BANK; t++){
        status[t] = IDLE;
    }

    for(int i = 0; i < size*TREES_PER_BANK;){
        if(!feedbackStream.empty()){
            std::cout << "read feedback" << std::endl;
            FetchRequest request = feedbackStream.read();
            if(request.needNewPage){
                fetchRequestStream.write(request);
            } else if(request.done){
                status[request.treeID] = IDLE;
                processCounter++;
                i++;
            }
        }
        for(int t = 0; t < TREES_PER_BANK; t++){
            if(status[t] == IDLE){
                if(!splitFeatureStream[t].empty()){
                    std::cout << "New Request" << std::endl;
                    FetchRequest newRequest{splitFeatureStream[t].read(), 0, t, false, false};
                    fetchRequestStream.write(newRequest);
                    #ifdef __SYNTHESIS__
                    status[t] = PROCESSING;
                    #else
                    i++;
                    if(processCounter++ == UPDATE_FEQUENCY){
                        newRequest.updateSmlBank = true;
                        fetchRequestStream.write(newRequest);
                    }
                    #endif
                    
                } 
            }
        }
        if(processCounter > UPDATE_FEQUENCY){
            processCounter = 0;
            FetchRequest newRequest;
            newRequest.updateSmlBank = true;
            fetchRequestStream.write(newRequest);
        }
    }
    FetchRequest shutdownRequest;
    shutdownRequest.shutdown = true;
    fetchRequestStream.write(shutdownRequest);
}