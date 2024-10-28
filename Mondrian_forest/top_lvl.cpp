#include "top_lvl.hpp"
#include "training.hpp"
#include "rng.hpp"
#include <hls_stream.h>
#include <stdint.h>

extern "C" {

void top_lvl(
    hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    hls::stream<label_vector> &outputStream
) {
    #pragma HLS INTERFACE port=inputFeatureStream mode=axis
    #pragma HLS INTERFACE port=inputLabelStream mode=axis
    #pragma HLS INTERFACE port=outputStream mode=axis
    #pragma HLS DATAFLOW

    #pragma HLS BIND_STORAGE variable=nodePool type=ram_2p
    #pragma HLS BIND_STORAGE variable=treePool type=ram_2p

    #pragma HLS ARRAY_PARTITION variable=nodePool cyclic factor=4 dim=1

    hls::stream<Tree> treeInfoStreams[TREE_COUNT];
    hls::stream<Tree> treeOutputStreams[TREE_COUNT];
    hls::stream<fixed_point> rngStream("rngStream");
    
    #pragma HLS STREAM variable=treeInfoStreams depth=4
    #pragma HLS STREAM variable=treeOutputStreams depth=4


    generate_rng(rngStream);

    for (int i = 0; i < TREE_COUNT; i++) {
        #pragma HLS PIPELINE II=1
        treeInfoStreams[i].write(treePool[i]);
        freeNodeIDs[i] = i*2;
    }

    for (int i = 0; i < TREE_COUNT; i++){
        #pragma HLS UNROLL
        train(inputFeatureStream, inputLabelStream, rngStream, treeInfoStreams[i], treeOutputStreams[i], nodePool, freeNodeIDs[i]);
    }

    for (int i = 0; i < TREE_COUNT; i++) {
        if(freeNodeIDs[i] == -1){
            freeNodeIDs[i] = globalNodeCounter++ * 2;
        }
    }

    for (int i = 0; i < TREE_COUNT; i++) {
        #pragma HLS PIPELINE II=1
        treePool[i] = treeOutputStreams[i].read();
    }

    outputStream.write(1);

}

}