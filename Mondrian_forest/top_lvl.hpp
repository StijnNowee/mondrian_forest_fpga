#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"
#include <hls_stream.h>

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &inferenceOutputStream,
    const InputSizes &sizes,
    PageBank pageBanks[BANK_COUNT]
);

void convertInputToVector(const input_t &raw, input_vector &input);

#endif /* TOP_LVL_H_ */