#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"
#include <hls_stream.h>

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &resultOutputStream,
    const InputSizes &sizes,
    PageBank readwrite[BANK_COUNT],
    PageBank readread[BANK_COUNT]
);

#endif /* TOP_LVL_H_ */