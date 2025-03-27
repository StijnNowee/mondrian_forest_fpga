#include "inference.hpp"

void traversal(hls::stream<input_vector> &inferenceStream, hls::stream<ClassDistribution> traversalOutputStream[TREES_PER_BANK], hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream, const int size);
void inference_per_tree(const feature_vector feature, const tree_t &tree, hls::stream<ClassDistribution> &inferenceOutputStream);
void copy_distribution(const Node_sml &from, ClassDistribution &to);

void voter(hls::stream<ClassDistribution> traversalOutputStream[TREES_PER_BANK],  hls::stream<ClassDistribution> &resultOutputStream, const int size);

void inference(hls::stream<input_vector> &inferenceStream, hls::stream<ClassDistribution> &inferenceOutputStream, hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream, const int size)
{   
    #pragma hls DATAFLOW
    hls::stream<ClassDistribution> traversalOutputStream[TREES_PER_BANK];

    traversal(inferenceStream, traversalOutputStream, smlTreeStream, treeUpdateCtrlStream, size);
    voter(traversalOutputStream, inferenceOutputStream, size);
}

void traversal(hls::stream<input_vector> &inferenceStream, hls::stream<ClassDistribution> traversalOutputStream[TREES_PER_BANK], hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream, const int size)
{
    for(int i = 0 ; i < size;){
        treeUpdateCtrlStream.read();
        hls::read_lock<trees_t> trees(smlTreeStream);
        while(true){
            if(!treeUpdateCtrlStream.empty()) break;
            if(!inferenceStream.empty()){
                std::cout << "Read inference" << std::endl;
                const input_vector newInput = inferenceStream.read();
                #pragma HLS ARRAY_PARTITION variable=newInput.feature complete
                i++;
                for(int t = 0; t < TREES_PER_BANK; t++){
                    #pragma HLS UNROLL
                    inference_per_tree(newInput.feature, trees[t], traversalOutputStream[t]);
                }
            } 
            #ifndef __SYNTHESIS__ 
            else{
                break;
            }
            #endif
        }
    }

}

void inference_per_tree(const feature_vector feature, const tree_t &tree, hls::stream<ClassDistribution> &inferenceOutputStream)
{
    bool done = false;
    Node_sml node = tree[0];
    #pragma HLS ARRAY_PARTITION variable=node.storage dim=1 type=complete
    while(!done){
        if(node.leaf()){
            done = true;
            ClassDistribution distributionStruct;
            copy_distribution(node, distributionStruct);
            
            inferenceOutputStream.write(distributionStruct);
        }else{
            node = (feature[node.feature()] > node.threshold()) ? tree[node.rightChild()] : tree[node.leftChild()];
        }
    }
}

void copy_distribution(const Node_sml &from, ClassDistribution &to)
{
    #pragma HLS ARRAY_PARTITION variable=from.storage complete
    #pragma HLS ARRAY_PARTITION variable=to complete

    for(int i = 0; i < CLASS_COUNT; i++)
    {
        #pragma HLS UNROLL
        to.distribution[i].range(7,0) = from.classDistribution(i).range(7,0);
    }
}

void voter(hls::stream<ClassDistribution> traversalOutputStream[TREES_PER_BANK],  hls::stream<ClassDistribution> &resultOutputStream, const int size)
{
    static const ap_ufixed<24,0> reciprocal = 1.0 / TREES_PER_BANK;
    for (int i = 0; i < size; i++) {
        ap_ufixed<24, 16> classSums[CLASS_COUNT] = {0};
        for(int t = 0; t < TREES_PER_BANK; t++){
            #pragma HLS UNROLL off
            ClassDistribution distribution = traversalOutputStream[t].read();
            for(int c = 0; c < CLASS_COUNT; c++){
                #pragma HLS PIPELINE II=1
                classSums[c] += distribution.distribution[c];
            }
        }
        ClassDistribution avg;
        #pragma HLS ARRAY_PARTITION variable=avg.distribution dim=1 type=complete
        
        for(int c = 0; c < CLASS_COUNT; c++){
            #pragma HLS UNROLL
            avg.distribution[c] = classSums[c] * reciprocal;
        }
        resultOutputStream.write(avg);
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