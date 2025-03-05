#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

void top_lvl(
    hls::stream<input_t> &trainInputStream,
    hls::stream<input_t>  &inferenceInputStream,
    hls::stream<ap_uint<50>> &inferenceOutputStream,
    const int size,
    // Page *pageBank1,
    Page *pageBank1
);

void convertInputToVector(const input_t &raw, input_vector &input);

#endif /* TOP_LVL_H_ */