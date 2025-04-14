#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"
#include <hls_stream.h>

void top_lvl(
    hls::stream<input_vector> inputStream[2],
    hls::stream<Result> &resultOutputStream,
    const InputSizes &sizes,
    PageBank trainHBM[BANK_COUNT],
    PageBank inferenceHBM[BANK_COUNT]
);

#endif /* TOP_LVL_H_ */