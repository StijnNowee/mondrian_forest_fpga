#include "top_lvl.hpp"
#include "training.hpp"
#include "rng.hpp"
#include <cstdint>
#include <hls_math.h>
#include <hls_stream.h>
#include <stdint.h>

extern "C" {
void read_and_train(
    hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Node_hbm *nodeBank1,
    Node_hbm *nodeBank2);

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


    treePool[0].root = 0;
    treePool[1].root = 0;

    read_and_train(inputFeatureStream, inputLabelStream, nodeBank1, nodeBank2);

}

void read_and_train(
    hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Node_hbm *nodeBank1,
    Node_hbm *nodeBank2)
    {
        #pragma HLS INLINE off
        feature_vector feature = inputFeatureStream.read();
        label_vector label = inputLabelStream.read();
        #pragma HLS ARRAY_PARTITION variable=feature complete dim=1

        train(feature, label, treePool[0], nodeBank1);
        train(feature, label, treePool[1], nodeBank2);

    }

}