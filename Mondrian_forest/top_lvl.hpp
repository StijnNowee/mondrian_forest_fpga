#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"
#include "hls_np_channel.h"

void top_lvl(
    hls::stream<feature_vector> &inputFeatureStream,
    Page *pageBank1_read,
    Page *pageBank2_read
// hls::stream<label_vector> &outputStream);
);



void split_feature(hls::stream<feature_vector> &in, hls::stream<feature_vector> &out);


#endif /* TOP_LVL_H_ */