#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &inferenceOutputStream1,
    hls::stream<Result> &inferenceOutputStream2,
    const int size,
    Page *pageBank1,
    Page *pageBank2
);

void convertInputToVector(const input_t &raw, input_vector &input);

#endif /* TOP_LVL_H_ */