#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"
#include <hls_stream.h>

void top_lvl(
    hls::stream<input_vector> inputStream[2],
    hls::stream<Result> &resultOutputStream,
    hls::stream<int> executionCountStream[BANK_COUNT],
    int maxPageNr[BANK_COUNT][TREES_PER_BANK],
    int sizes[2],
    PageBank trainHBM[BANK_COUNT],
    PageBank inferenceHBM[BANK_COUNT]
);

#endif /* TOP_LVL_H_ */