#include "train.hpp"

// void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream)
void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, hls::stream<int> &processTreeStream)
{
    static TreeStatus status[TREES_PER_BANK] = {IDLE};
    //#pragma HLS BIND_STORAGE variable=status type=ram_1p 
    static int treeUpdateCount[TREES_PER_BANK] = {0};
    //#pragma HLS BIND_STORAGE variable=treeUpdateCount type=ram_1p 
    if(!feedbackStream.empty()){
        FetchRequest request = feedbackStream.read();
        if(request.needNewPage){
            fetchRequestStream.write(request);
        }else{
            if(treeUpdateCount[request.treeID]++ > 10){
                treeUpdateCount[request.treeID] = 0;
                //processTreeStream.write(request.treeID);
            }
            status[request.treeID] = IDLE;
            //processTreeStream.write(request.treeID);
        }
    }
    check_idle_trees: for(int t = 0; t < TREES_PER_BANK; t++){
        if(status[t] == IDLE && !splitFeatureStream[t].empty()){
            status[t] = TRAINING;
            FetchRequest request{splitFeatureStream[t].read(), 0, t, false, false, false};
            fetchRequestStream.write(request);
        }
    }
}