#include "top_lvl.hpp"
#include "rng.hpp"

#include <cstdint>
#include <hls_math.h>
#include <hls_stream.h>
#include <stdint.h>
#include "control_unit.hpp"
#include "hls_task.h"

void top_lvl(
    feature_vector *inputFeature,
    Page pageBank1_read[MAX_PAGES],
    Page pageBank2_read[MAX_PAGES]
) {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE mode=s_axilite port=inputFeature
    #pragma HLS INTERFACE m_axi port=pageBank1_read bundle=hbm0 depth=MAX_PAGES  max_write_burst_length=256 max_read_burst_length=256
    #pragma HLS INTERFACE m_axi port=pageBank2_read bundle=hbm1 depth=MAX_PAGES  max_write_burst_length=256 max_read_burst_length=256


    hls::split::load_balance<unit_interval, 4, 20> rngStream;
    
    generate_rng(rngStream.in);

    control_unit(inputFeature, pageBank1_read, pageBank2_read, rngStream);

    
    

}