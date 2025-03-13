#include "inference.hpp"
#include "top_lvl.hpp"

void traversal(hls::stream<input_vector> &inferenceStream, hls::stream<ClassDistribution> traversalOutputStream[TREES_PER_BANK], hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream, const int size);
void inference_per_tree(const input_vector &input, const tree_t &tree, hls::stream<ClassDistribution> &inferenceOutputStream);
void copy_distribution(classDistribution_t &from, ClassDistribution &to);

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
        std::cout << "traversal entry" << std::endl;
        hls::read_lock<trees_t> trees(smlTreeStream); 
        treeUpdateCtrlStream.read();
        while(true){
            if(!treeUpdateCtrlStream.empty()) break;
            if(!inferenceStream.empty()){
                std::cout << "should work" << std::endl;
                auto newInput = inferenceStream.read();
                i++;
                for(int t = 0; t < TREES_PER_BANK; t++){
                    #pragma hls UNROLL
                    inference_per_tree(newInput, trees[t], traversalOutputStream[t]);
                }
            } 
            #ifndef __SYNTHESIS__ 
            else{
                break;
            }
            #endif
        }
        std::cout << "traverse done" << std::endl;
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
        to.distribution[i].range(8,0) = from[i].range(8,0);
    }
}

void voter(hls::stream<ClassDistribution> traversalOutputStream[TREES_PER_BANK],  hls::stream<ClassDistribution> &resultOutputStream, const int size)
{
    ClassDistribution distributions[TREES_PER_BANK];
    for (int i = 0; i < size; i++) {
        for(int t = 0; t < TREES_PER_BANK; t++){
            distributions[t] = traversalOutputStream[t].read();
        }
        ap_ufixed<24, 16> classSums[CLASS_COUNT] = {0};
        for(int t = 0; t < TREES_PER_BANK; t++){
            for(int c = 0; c < CLASS_COUNT; c++){
                classSums[c] += distributions[t].distribution[c];
                std::cout << "Tree: " << t << " Class: " << c << "Distribution: " << distributions[t].distribution[c].to_float() << "Sum: " << classSums[c].to_float() << std::endl;
            }
        }
        ClassDistribution avg;
        for(int c = 0; c < CLASS_COUNT; c++){
            avg.distribution[c] = classSums[c]/TREES_PER_BANK;
            std::cout << "Class: " << c << " Average: " << avg.distribution[c].to_float() << std::endl; 
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