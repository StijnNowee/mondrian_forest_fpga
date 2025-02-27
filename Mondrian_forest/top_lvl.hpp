#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<node_t> &outputStream,
    hls::stream<bool> &controlOutputStream,
    hls::stream<Result> &resultOutputStream,
    //Page *pageBank1,
    Page *pageBank1,
    const int size
);

void convertInputToVector(const input_t &raw, input_vector &input);

#endif /* TOP_LVL_H_ */