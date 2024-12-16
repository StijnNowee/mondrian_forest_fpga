#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"
#include "hls_np_channel.h"

void top_lvl(
    feature_vector *inputFeature,
    Page pageBank1_read[MAX_PAGES],
    Page pageBank2_read[MAX_PAGES]
// hls::stream<label_vector> &outputStream);
);

// void featureDistributer(hls::stream<feature_vector> &inputFeatureStream, hls::stream<feature_vector> &newFeatureStream);

// void split_feature(hls::stream<feature_vector> &in, hls::stream<feature_vector> &out);


#endif /* TOP_LVL_H_ */