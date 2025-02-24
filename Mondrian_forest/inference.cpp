#include "inference.hpp"
#include "controller.hpp"
#include "hls_task.h"

void run_inference(hls::stream<input_vector> &inferenceStream, Node_sml trees[TREES_PER_BANK][MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE], hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK]);
void inference_per_tree(const input_vector &input, Node_sml tree[MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE], hls::stream<ClassDistribution> &inferenceOutputStream);
void copy_distribution(classDistribution_t &from, ClassDistribution &to);

// void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream, hls::stream<Result> &resultOutputStream, const Page *pagePool)
void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<int> &processTreeStream, hls::stream<Result> &resultOutputStream, const Page *pagePool)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_ctrl_none port=return

    Node_sml trees[TREES_PER_BANK][MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE];
    #pragma HLS ARRAY_PARTITION variable=trees dim=1 type=complete

    hls_thread_local hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK];

    //hls_thread_local hls::task t1(controller, processTreeStream, processDoneStream, pagePool, trees);
    hls_thread_local hls::task t1(controller, processTreeStream, pagePool, trees);
    hls_thread_local hls::task t2(run_inference, inferenceInputStream, trees, inferenceOutputstreams);
    
}

void run_inference(hls::stream<input_vector> &inferenceStream, Node_sml trees[TREES_PER_BANK][MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE], hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK])
{
    if(!inferenceStream.empty()){
        input_vector input = inferenceStream.read();
        for(int i = 0; i < TREES_PER_BANK; i++){
            inference_per_tree(input, trees[i], inferenceOutputstreams[i]);
        }
    }
}

void inference_per_tree(const input_vector &input, Node_sml tree[MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE], hls::stream<ClassDistribution> &inferenceOutputStream)
{
    bool done = false;
    Node_sml node = tree[0];
    #pragma HLS ARRAY_PARTITION variable=node dim=1 type=complete
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