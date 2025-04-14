#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"
#include <hls_stream.h>

void top_lvl(
    hls::stream<input_vector> inputStream[2],
    hls::stream<Result> &resultOutputStream,
    const InputSizes &sizes,
    Page* trainHBM,
    Page* inferenceHBM
);

#endif /* TOP_LVL_H_ */