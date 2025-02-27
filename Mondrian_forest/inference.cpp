#include "inference.hpp"
#include "hls_task.h"
#include "top_lvl.hpp"

void run_inference(hls::stream<int> &processTreeStream, hls::stream<input_t> &inferenceStream, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK]);
void inference_per_tree(const input_vector &input, const tree_t &tree, hls::stream<ClassDistribution> &inferenceOutputStream);
void copy_distribution(classDistribution_t &from, ClassDistribution &to);

void voter(hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK],  hls::stream<Result> &resultOutputStream);

// void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream, hls::stream<Result> &resultOutputStream, const Page *pagePool)
void inference(hls::stream<input_t> &inferenceInputStream, hls::stream<Result> &resultOutputStream, const Page *pagePool)
{
    #pragma HLS DATAFLOW
    #pragma HLS STABLE variable=pagePool
    //#pragma HLS INTERFACE ap_ctrl_hs port=return
    #pragma HLS INTERFACE ap_ctrl_none port=return

    //treesml_t trees[TREES_PER_BANK];
    //#pragma HLS stream type=pipo variable=trees depth=2
    hls_thread_local hls::stream_of_blocks<trees_t, 2> treeStream;
    //#pragma HLS BIND_STORAGE variable=trees type=ram_2p

    hls_thread_local hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK];
    hls_thread_local hls::stream<int> processTreeStream("ProcessTreeStream");

    //hls_thread_local hls::task t1(controller, processTreeStream, processDoneStream, pagePool, trees);
    hls_thread_local hls::task t1(condenser, processTreeStream, pagePool, treeStream);
    hls_thread_local hls::task t2(run_inference, processTreeStream, inferenceInputStream, treeStream, inferenceOutputstreams);
    hls_thread_local hls::task t3(voter, inferenceOutputstreams, resultOutputStream);
    
    
}

void run_inference(hls::stream<int> &processTreeStream, hls::stream<input_t> &inferenceStream, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK])
{
    hls::read_lock<trees_t> trees(treeStream);
    while(treeStream.empty()){
        auto rawInput = inferenceStream.read();
        input_vector newInput;
        convertInputToVector(rawInput, newInput);
        processTreeStream.write(0);
        for(int i = 0; i < TREES_PER_BANK; i++){
            inference_per_tree(newInput, trees[i], inferenceOutputstreams[i]);
        }
    }
}

void inference_per_tree(const input_vector &input, const tree_t &tree, hls::stream<ClassDistribution> &inferenceOutputStream)
{
    bool done = false;
    Node_sml node = tree[0];
    while(!done){
        if(node.leaf){
            done = true;
            ClassDistribution distributionStruct;
            copy_distribution(node.classDistribution, distributionStruct);
            inferenceOutputStream.write(distributionStruct);
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