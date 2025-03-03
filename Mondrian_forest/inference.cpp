#include "inference.hpp"
#include "hls_task.h"
#include "top_lvl.hpp"
#include <etc/autopilot_ssdm_op.h>

void run_inference(hls::stream<input_t> &inferenceStream, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<ap_uint<50>> &inferenceOutputStreams,  hls::stream<bool> &treeUpdateCtrlStream);
void inference_per_tree(const input_vector &input, const tree_t &tree, hls::stream<ap_uint<50>> &inferenceOutputStream);
void copy_distribution(classDistribution_t &from, ClassDistribution &to);

//void voter(hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK],  hls::stream<Result> &resultOutputStream);

void inference(hls::stream<input_t> &inferenceInputStream, hls::stream<ap_uint<50>> &inferenceOutputStream, hls::stream_of_blocks<trees_t> &treeStream,  hls::stream<bool> &treeUpdateCtrlStream)
{
    // #pragma HLS DATAFLOW
    // #pragma HLS INTERFACE ap_ctrl_none port=return
    #pragma HLS inline


    //hls_thread_local hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK];

    
    hls_thread_local hls::task t2(run_inference, inferenceInputStream, treeStream, inferenceOutputStream, treeUpdateCtrlStream);
    //hls_thread_local hls::task t3(voter, inferenceOutputstreams, resultOutputStream);
    
    
}

void run_inference(hls::stream<input_t> &inferenceStream, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<ap_uint<50>> &inferenceOutputStream,  hls::stream<bool> &treeUpdateCtrlStream)//hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK])
{
    hls::read_lock<trees_t> trees(treeStream); 
    treeUpdateCtrlStream.read();
    std::cout << "inference: " << !treeStream.empty() << std::endl;
    while(treeUpdateCtrlStream.empty()){
        if(!inferenceStream.empty()){
            std::cout << "Read from inferenceStream" << std::endl;
            auto rawInput = inferenceStream.read();
            input_vector newInput;
            convertInputToVector(rawInput, newInput);
            for(int i = 0; i < TREES_PER_BANK; i++){
                inference_per_tree(newInput, trees[i], inferenceOutputStream);
            }
        }
    }
    
}

void inference_per_tree(const input_vector &input, const tree_t &tree, hls::stream<ap_uint<50>> &inferenceOutputStream)
{
    bool done = false;
    Node_sml node = tree[0];
    while(!done){

        if(node.leaf){
            done = true;
            ClassDistribution distributionStruct;
            copy_distribution(node.classDistribution, distributionStruct);
            ap_uint<50> output = distributionStruct.distribution[0];
            inferenceOutputStream.write(output);
            std::cout << "Write" << std::endl;
        }else{
            node = (input.feature[node.feature] > node.threshold) ? tree[node.rightChild] : tree[node.leftChild];
        }
    }
}

void copy_distribution(classDistribution_t &from, ClassDistribution &to)
{
    #pragma HLS ARRAY_PARTITION variable=from complete
    #pragma HLS ARRAY_PARTITION variable=to complete

    for(int i = 0; i < CLASS_COUNT; i++)
    {
        #pragma HLS UNROLL
        to.distribution[i] = from[i];
    }
}

void voter(hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK],  hls::stream<Result> &resultOutputStream)
{
    if(!inferenceOutputstreams[0].empty()){
        ClassDistribution dis = inferenceOutputstreams[0].read();
        Result result;
        for(int i = 0; i < CLASS_COUNT; i++){
            if(dis.distribution[i] > result.confidence){
                result.resultClass = i;
                result.confidence = dis.distribution[i];
            }
        }
        resultOutputStream.write(result);
    }
}