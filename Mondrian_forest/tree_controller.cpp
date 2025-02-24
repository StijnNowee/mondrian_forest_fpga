#include "train.hpp"

// void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream)
void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, hls::stream<int> &processTreeStream)
{
    TreeStatus status[TREES_PER_BANK];
    //#pragma HLS BIND_STORAGE variable=status type=ram_1p 
    int treeUpdateCount[TREES_PER_BANK];
    //#pragma HLS BIND_STORAGE variable=treeUpdateCount type=ram_1p 
    if(!feedbackStream.empty()){
        FetchRequest request = feedbackStream.read();
        if(request.needNewPage){
            fetchRequestStream.write(request);
        }else{
            if(treeUpdateCount[request.treeID]++ > 10){
                treeUpdateCount[request.treeID] = 0;
                processTreeStream.write(request.treeID);
            }else{
                status[request.treeID] = IDLE;
            }
        }
    } else{
        check_idle_trees: for(int t = 0; t < TREES_PER_BANK; t++){
            if(status[t] == IDLE && !splitFeatureStream[t].empty()){
                status[t] = TRAINING;
                FetchRequest request{splitFeatureStream[t].read(), 0, t, false, false, false};
                fetchRequestStream.write(request);
            }
        }
    }
    // if(!processDoneStream.empty()){
    //     int treeID = processDoneStream.read();
    //     status[treeID] = IDLE;
    // }
}