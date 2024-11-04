#include "top_lvl.hpp"
#include "training.hpp"
#include "rng.hpp"
#include <cstdint>
#include <hls_math.h>
#include <hls_stream.h>
#include <stdint.h>

extern "C" {
void initialize();

void top_lvl(
    hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Node_hbm *nodeBank1,
    Node_hbm *nodeBank2,
    hls::stream<label_vector> &outputStream
) {
    #pragma HLS INTERFACE port=inputFeatureStream mode=axis
    #pragma HLS INTERFACE port=inputLabelStream mode=axis
    #pragma HLS INTERFACE m_axi port=nodeBank1 bundle=gmem0 depth=64
    #pragma HLS INTERFACE m_axi port=nodeBank2 bundle=gmem1 depth=64
    #pragma HLS INTERFACE port=outputStream mode=axis
    #pragma HLS DATAFLOW


    // #pragma HLS AGGREGATE variable=nodeBank1 compact=auto
    // #pragma HLS AGGREGATE variable=nodeBank2 compact=auto
    //#pragma HLS ARRAY_PARTITION variable=nodePool cyclic factor=TREE_COUNT dim=1

    // hls::stream<Tree> treeInfoStreams[TREE_COUNT];
    // hls::stream<Tree> treeOutputStreams[TREE_COUNT];
    //hls::stream<fixed_point> rngStream("rngStream");
    
    // #pragma HLS STREAM variable=treeInfoStreams depth=4
    // #pragma HLS STREAM variable=treeOutputStreams depth=4
    //initialize();

    //generate_rng(rngStream);

    // for (int i = 0; i < TREE_COUNT; i++) {
    //     #pragma HLS PIPELINE II=1
    //     treeInfoStreams[i].write(treePool[i]);
    // }

    feature_vector feature = inputFeatureStream.read();
    label_vector label = inputLabelStream.read();
    #pragma HLS ARRAY_PARTITION variable=feature complete dim=1

    //hls::stream<Node_hbm> nodeStream("nodeStream");

    // Train_loop: for (int i = 0; i < TREE_COUNT; i++){
    //     #pragma HLS UNROLL
        train(feature, label, treePool[0], nodeBank1);
        train(feature, label, treePool[1], nodeBank2);
    // }

    // for (int i = 0; i < TREE_COUNT; i++) {
    //     #pragma HLS PIPELINE II=1
    //     treePool[i] = treeOutputStreams[i].read();
    // }

    //outputStream.write(1);

}

// void initialize()
// {
//     Tree_init: for(int i = 0; i < TREE_COUNT; i++){
//         treePool[i].root = 0;
//     }
// }

}