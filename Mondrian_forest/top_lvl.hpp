#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page *pageBank1,
    hls::stream<unit_interval, 100> &rngStream1,
    hls::stream<unit_interval, 100> &rngStream2,
    int size
);


#endif /* TOP_LVL_H_ */