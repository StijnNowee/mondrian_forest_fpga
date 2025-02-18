#include "inference.hpp"
#include "controller.hpp"

void run_inference(hls::stream<input_vector> &inferenceStream, Node_sml trees[TREES_PER_BANK][MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE]);
void inference_per_tree(const input_vector &input, Node_sml tree[MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE]);

void inference(hls::stream<input_vector> &inferenceStream, hls::stream<int> &processTreeStream, const Page *pagePool)
{
    #pragma HLS DATAFLOW

    Node_sml trees[TREES_PER_BANK][MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE];
    #pragma HLS ARRAY_PARTITION variable=trees dim=1 type=complete

    controller(processTreeStream, pagePool, trees);
    run_inference(inferenceStream, trees);
    
}

void run_inference(hls::stream<input_vector> &inferenceStream, Node_sml trees[TREES_PER_BANK][MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE])
{
    if(!inferenceStream.empty()){
        input_vector input = inferenceStream.read();
        for(int i = 0; i < TREES_PER_BANK; i++){
            inference_per_tree(input, trees[i]);
        }
    }
}

void inference_per_tree(const input_vector &input, Node_sml tree[MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE])
{
    bool done = false;
    while(!done){

    }
}