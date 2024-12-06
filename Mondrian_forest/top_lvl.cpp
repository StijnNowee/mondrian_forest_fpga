#include "top_lvl.hpp"
#include "rng.hpp"
#include "hls_np_channel.h"
#include <cstdint>
#include <hls_math.h>
#include <hls_stream.h>
#include <stdint.h>
#include "control_unit.hpp"

void top_lvl(
    hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Page *pageBank1_read,
    Page *pageBank1_write,
    Page *pageBank2_read,
    Page *pageBank2_write
) {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE port=inputFeatureStream mode=axis
    #pragma HLS INTERFACE port=inputLabelStream mode=axis
    #pragma HLS INTERFACE m_axi port=pageBank1_write bundle=hbm0 depth=MAX_PAGES  max_write_burst_length=256 max_read_burst_length=256 channel=0
    #pragma HLS INTERFACE m_axi port=pageBank1_read bundle=hbm0 depth=MAX_PAGES  max_write_burst_length=256 max_read_burst_length=256 channel=1
    #pragma HLS INTERFACE m_axi port=pageBank2_write bundle=hbm1 depth=MAX_PAGES  max_write_burst_length=256 max_read_burst_length=256 channel=0
    #pragma HLS INTERFACE m_axi port=pageBank2_read bundle=hbm1 depth=MAX_PAGES  max_write_burst_length=256 max_read_burst_length=256 channel=1

    //Tree treePool[2];
    //#pragma HLS ARRAY_PARTITION variable=treePool dim=1 type=complete

    //hls::split::load_balance<unit_interval, 2, 20> rngStream;
    //hls::stream<unit_interval, 100> rngStream1("rngStream1");
    //hls::stream<unit_interval, 100> rngStream2("rngStream2");
    //hls::stream<int> rootNodeStream1("rootNodeStream1");
    //hls::stream<int> rootNodeStream2("rootNodeStream2");

    //hls::split::round_robin<feature_vector, 2> featureStream("featureStream");
    hls::split::load_balance<unit_interval, 4, 20> rngStream;

    generate_rng(rngStream.in);
    control_unit(inputFeatureStream, inputLabelStream, pageBank1_read, pageBank1_write, pageBank2_read, pageBank2_write, rngStream);
    //split_feature(inputFeatureStream, featureStream.in);

    //featureStream.in.write(inputFeatureStream.read());
    // label_vector label = inputLabelStream.read();
    // rootNodeStream1.write(1);
    //rootNodeStream2.write(1);


    //train(inputFeatureStream, nodeBank1, rngStream1, rootNodeStream1);
    //train(featureStream.out[1], nodeBank2, rngStream2, rootNodeStream2);

}

// void split_feature(hls::stream<feature_vector> &in, hls::stream<feature_vector> &out)
// {
//     feature_vector feature = in.read();
//     out.write(feature);
// }