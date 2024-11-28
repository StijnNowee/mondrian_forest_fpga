#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

void top_lvl(
hls::stream<feature_vector> &inputFeatureStream, 
hls::stream<label_vector> &inputLabelStream, 
Node_hbm *nodeBank1, 
Node_hbm *nodeBank2
// hls::stream<label_vector> &outputStream);
);

void split_feature(hls::stream<feature_vector> &in, hls::stream<feature_vector> &out);


#endif /* TOP_LVL_H_ */