#include "top_lvl.hpp"
#include "rng.hpp"
#include <cstdint>
#include <hls_math.h>
#include <hls_stream.h>
#include <stdint.h>
#include "training.hpp"

void top_lvl(
    hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Node_hbm *nodeBank1,
    Node_hbm *nodeBank2
) {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE port=inputFeatureStream mode=axis
    #pragma HLS INTERFACE port=inputLabelStream mode=axis
    #pragma HLS INTERFACE m_axi port=nodeBank1 bundle=gmem0 depth=MAX_NODES num_read_outstanding=16 num_write_outstanding=16 
    #pragma HLS INTERFACE m_axi port=nodeBank2 bundle=gmem1 depth=MAX_NODES num_read_outstanding=16 num_write_outstanding=16 

    //Tree treePool[2];
    //#pragma HLS ARRAY_PARTITION variable=treePool dim=1 type=complete

    //hls::split::load_balance<unit_interval, 2, 20> rngStream;
    hls::stream<unit_interval, 100> rngStream1("rngStream1");
    hls::stream<unit_interval, 100> rngStream2("rngStream2");
    hls::stream<int> rootNodeStream1("rootNodeStream1");
    hls::stream<int> rootNodeStream2("rootNodeStream2");

    generate_rng(rngStream1, rngStream2);

    feature_vector feature = inputFeatureStream.read();
    label_vector label = inputLabelStream.read();
    #pragma HLS ARRAY_PARTITION variable=feature dim=1 type=complete
    rootNodeStream1.write(1);
    rootNodeStream2.write(1);


    train(feature, label, nodeBank1, rngStream1, rootNodeStream1);
    train(feature, label, nodeBank2, rngStream2, rootNodeStream2);

}
