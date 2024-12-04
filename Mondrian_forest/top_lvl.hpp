#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

void top_lvl(
    hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Page *pageBank1_read,
    Page *pageBank1_write,
    Page *pageBank2
// hls::stream<label_vector> &outputStream);
);

void split_feature(hls::stream<feature_vector> &in, hls::stream<feature_vector> &out);


#endif /* TOP_LVL_H_ */