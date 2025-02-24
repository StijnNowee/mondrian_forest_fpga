#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<node_t> &outputStream,
    hls::stream<Result> &resultOutputStream,
    Page *pageBank1
);


#endif /* TOP_LVL_H_ */