#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

void top_lvl(
    hls::stream<input_t> &inputFeatureStream,
    Page *pageBank1
);


#endif /* TOP_LVL_H_ */