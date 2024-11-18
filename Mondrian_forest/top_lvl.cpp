#include "top_lvl.hpp"
#include "training.hpp"
#include "hls_task.h"
#include "rng.hpp"
#include <cstdint>
#include <hls_math.h>
#include <hls_stream.h>
#include <stdint.h>
#include "hls_np_channel.h"

extern "C" {
// void read_and_train(
//     hls::stream<feature_vector> &inputFeatureStream,
//     hls::stream<label_vector> &inputLabelStream,
//     Node_hbm *nodeBank1,
//     Node_hbm *nodeBank2);

//void start_rng_task(hls::stream<unit_interval> &rngStream);

void top_lvl(
    hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Node_hbm *nodeBank1,
    Node_hbm *nodeBank2,
    hls::stream<label_vector> &outputStream
) {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE port=inputFeatureStream mode=axis
    #pragma HLS INTERFACE port=inputLabelStream mode=axis
    #pragma HLS INTERFACE m_axi port=nodeBank1 bundle=gmem0 depth=50
    #pragma HLS INTERFACE m_axi port=nodeBank2 bundle=gmem1 depth=50
    #pragma HLS INTERFACE port=outputStream mode=axis
    
    feature_vector feature = inputFeatureStream.read();
    label_vector label = inputLabelStream.read();
    #pragma HLS ARRAY_PARTITION variable=feature complete dim=1
    hls::split::load_balance<unit_interval, 2, 20> rngStream;

    //start_rng_task(rngStream.in);
    hls_thread_local hls::task t1(generate_rng, rngStream.in);
    hls_thread_local hls::task t2()

    train(feature, label, treePool[0], nodeBank1, rngStream.out[0]);
    train(feature, label, treePool[1], nodeBank2, rngStream.out[1]);

    //read_and_train(inputFeatureStream, inputLabelStream, nodeBank1, nodeBank2);

}

//void store_locally(feature_vector &feature, label_vector &label, )

// void read_and_train(
//     hls::stream<feature_vector> &inputFeatureStream,
//     hls::stream<label_vector> &inputLabelStream,
//     Node_hbm *nodeBank1,
//     Node_hbm *nodeBank2)
//     {
//         #pragma HLS DATAFLOW

        


//     }

// void start_rng_task(hls::stream<unit_interval> &rngStream)
// {
//     #pragma HLS INLINE off
//     #pragma HLS DATAFLOW
//     #pragma HLS INTERFACE mode=ap_ctrl_none port=return
//     hls_thread_local hls::task t1 (generate_rng, rngStream);
// }

}