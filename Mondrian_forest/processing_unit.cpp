#include "processing_unit.hpp"
#include "train.hpp"
#include "inference.hpp"
#include "top_lvl.hpp"
#include <hls_task.h>

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<input_vector> &inferenceInputStream, const int size);
void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, const int size);

void processing_unit(hls::stream<input_t> &inputFeatureStream, hls::stream<unit_interval> &rngStream, Page *pageBank, const InputSizes &sizes, hls::stream<ClassDistribution> &inferenceOutputStream)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE port=return mode=ap_ctrl_chain

    trees_t smlTreeBank;
    //#pragma HLS ARRAY_PARTITION variable=smlTreeBank dim=1 type=complete
    //#pragma HLS stream type=pipo variable=smlTreeBank depth=2
    
    hls_thread_local hls::stream<FetchRequest,5> feedbackStream("FeedbackStream");
    hls_thread_local hls::stream<FetchRequest,5> fetchRequestStream("FetchRequestStream");
    hls::stream<input_vector> inferenceInputStream("inferenceInputStream");
    hls_thread_local hls::stream<input_vector,3> splitFeatureStream[TREES_PER_BANK];
    feature_distributor(inputFeatureStream, splitFeatureStream, inferenceInputStream, sizes.total);
    tree_controller(splitFeatureStream, feedbackStream, fetchRequestStream, sizes.training);
    
    // hls_thread_local hls::stream_of_blocks<trees_t, 3> treeStream;
    // hls_thread_local hls::stream<bool> treeUpdateCtrlStream("TreeUpdateCtrlStream");

    train(fetchRequestStream, rngStream, feedbackStream, pageBank, smlTreeBank ,sizes.training);

    inference(inferenceInputStream, inferenceOutputStream, smlTreeBank, sizes.inference);
   
}

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<input_vector> &inferenceInputStream, const int size)
{
    for(int i = 0; i < size; i++){
        auto rawInput = newFeatureStream.read();
        input_vector newInput;
        convertInputToVector(rawInput, newInput);
        std::cout << "inferenceSample? " << newInput.inferenceSample << " Count: " << i << std::endl;
        if(newInput.inferenceSample){
            inferenceInputStream.write(newInput);
        }else{
            for(int t = 0; t < TREES_PER_BANK; t++){
                std::cout << "writeFeature: " << i << std::endl;
                splitFeatureStream[t].write(newInput);
            }
        }
    }
}

void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, const int size)
{
    TreeStatus status[TREES_PER_BANK];
    int freePageIndex[TREES_PER_BANK];
    int processCounter = 0;

    for(int t = 0; t < TREES_PER_BANK; t++){
        status[t] = IDLE;
        freePageIndex[t] = 1;
    }

    for(int i = 0; i < size*TREES_PER_BANK;){
        if(!feedbackStream.empty()){
            //std::cout << "read feedback" << std::endl;
            FetchRequest request = feedbackStream.read();
            if(request.needNewPage){
                fetchRequestStream.write(request);
            } else if(request.extraPage){
                freePageIndex[request.treeID]++;
            } else{
                status[request.treeID] = IDLE;
                processCounter++;
                std::cout << "return?" << std::endl;
                #ifdef __SYNTHESIS__
                i++;
                #endif
            }
        }
        for(int t = 0; t < TREES_PER_BANK; t++){
            if(status[t] == IDLE){
                if(!splitFeatureStream[t].empty()){
                    std::cout << "New Request:" << i << std::endl;
                    FetchRequest newRequest{splitFeatureStream[t].read(), 0, t, false, false};
                    newRequest.freePageIdx = freePageIndex[t];
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