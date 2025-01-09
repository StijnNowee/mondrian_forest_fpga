#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page pageBank1[MAX_PAGES],
    hls::stream<unit_interval, 20> &rngStream1,
    hls::stream<unit_interval, 20> &rngStream2
);


#endif /* TOP_LVL_H_ */