#include "top_lvl.hpp"
#include "rng.hpp"

#include <cstdint>
#include <stdint.h>
#include "control_unit.hpp"
#include "train.hpp"
#include "hls_np_channel.h"

void top_lvl(
    feature_vector *inputFeature,
    Page pageBank1[MAX_PAGES]
) {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE mode=s_axilite port=inputFeature
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES  max_write_burst_length=256 max_read_burst_length=256


    hls::split::load_balance<unit_interval, 2, 20> rngStream;
    hls::stream<FetchRequest> fetchRequestStream("FetchRequestStream");
    
    generate_rng(rngStream.in);

    control_unit(inputFeature, pageBank1, fetchRequestStream);

    hls::stream_of_blocks<IPage> fetchOutput;
    hls::stream_of_blocks<IPage> traverseOutput;
    hls::stream_of_blocks<IPage> splitterOut;

    pre_fetcher(fetchRequestStream, fetchOutput, pageBank1);
    tree_traversal( fetchOutput, rngStream.out[0], traverseOutput);
    splitter(traverseOutput, rngStream.out[1], splitterOut);
    save(splitterOut, pageBank1);
    

}