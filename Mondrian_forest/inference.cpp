#include "inference.hpp"
#include "top_lvl.hpp"

void run_inference(hls::stream<input_t> &inferenceStream, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<Result> &inferenceOutputStreams,  hls::stream<bool> &treeUpdateCtrlStream);
void inference_per_tree(const input_vector &input, const tree_t &tree, hls::stream<Result> &inferenceOutputStream);
void copy_distribution(classDistribution_t &from, ClassDistribution &to);

//void voter(hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK],  hls::stream<Result> &resultOutputStream);

void inference(hls::stream<input_t> &inferenceInputStream, hls::stream<Result> &inferenceOutputStream, hls::stream_of_blocks<trees_t> &treeStream,  hls::stream<bool> &treeUpdateCtrlStream)
{   
    run_inference(inferenceInputStream, treeStream, inferenceOutputStream, treeUpdateCtrlStream);
}

void run_inference(hls::stream<input_t> &inferenceStream, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<Result> &inferenceOutputStream,  hls::stream<bool> &treeUpdateCtrlStream)//hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK])
{
    if(!treeStream.empty()){
        hls::read_lock<trees_t> trees(treeStream); 
        treeUpdateCtrlStream.read();
        while(true){
            if(!inferenceStream.empty()){
                auto rawInput = inferenceStream.read();
                input_vector newInput;
                convertInputToVector(rawInput, newInput);
                for(int i = 0; i < TREES_PER_BANK; i++){
                    #pragma hls UNROLL
                    inference_per_tree(newInput, trees[i], inferenceOutputStream);
                }
            }
            if(!treeUpdateCtrlStream.empty()) break;
        }
    }
}

void inference_per_tree(const input_vector &input, const tree_t &tree, hls::stream<Result> &inferenceOutputStream)
{
    bool done = false;
    Node_sml node = tree[0];
    while(!done){
        if(node.leaf){
            done = true;
            ClassDistribution distributionStruct;
            copy_distribution(node.classDistribution, distributionStruct);
            Result output;
            for(int i = 0; i < CLASS_COUNT; i++){
                output.distribution[i].range(8,0) = node.classDistribution[i].range(8,0);
            }
            inferenceOutputStream.write(output);
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

// void voter(hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK],  hls::stream<Result> &resultOutputStream)
// {
//     if(!inferenceOutputstreams[0].empty()){
//         ClassDistribution dis = inferenceOutputstreams[0].read();
//         Result result;
//         for(int i = 0; i < CLASS_COUNT; i++){
//             if(dis.distribution[i] > result.confidence){
//                 result.resultClass = i;
//                 result.confidence = dis.distribution[i];
//             }
//         }
//         resultOutputStream.write(result);
//     }
// }