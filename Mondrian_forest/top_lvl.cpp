#include "top_lvl.hpp"
#include "rng.hpp"
#include <cstdint>
#include <hls_math.h>
#include <hls_stream.h>
#include <stdint.h>
#include "hls_np_channel.h"
#include "training.hpp"

extern "C" {
// void read_and_train(
//     hls::stream<feature_vector> &inputFeatureStream,
//     hls::stream<label_vector> &inputLabelStream,
//     Node_hbm *nodeBank1,
//     Node_hbm *nodeBank2);

// void read_and_train(Tree *treePool, Node_hbm *nodeBank1, Node_hbm *nodeBank2,hls::split::load_balance<unit_interval, 2, 20> &rngStream);
// void generate_task(hls::split::load_balance<unit_interval, 2, 20> &rngStream);

//void start_rng_task(hls::stream<unit_interval> &rngStream);

void top_lvl(
    hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Node_hbm *nodeBank1,
    Node_hbm *nodeBank2
) {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE port=inputFeatureStream mode=axis
    #pragma HLS INTERFACE port=inputLabelStream mode=axis
    #pragma HLS INTERFACE m_axi port=nodeBank1 bundle=gmem0 depth=50
    #pragma HLS INTERFACE m_axi port=nodeBank2 bundle=gmem1 depth=50

    Tree treePool[2];
    #pragma HLS ARRAY_PARTITION variable=treePool dim=1 type=complete
    
    hls::split::load_balance<unit_interval, 2, 20> rngStream;

    generate_rng(rngStream.in);

    feature_vector feature = inputFeatureStream.read();
    label_vector label = inputLabelStream.read();
    #pragma HLS ARRAY_PARTITION variable=feature dim=1 type=complete

    train(feature, label, treePool[0], nodeBank1, rngStream.out[0]);
    train(feature, label, treePool[1], nodeBank2, rngStream.out[1]);

    }

}