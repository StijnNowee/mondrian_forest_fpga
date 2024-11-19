#ifndef CONTROL_UNIT_H_
#define CONTROL_UNIT_H_
#include "common.hpp"
#include "hls_np_channel.h"
void control_unit(hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Tree *treePool,
    Node_hbm *nodeBank1,
    Node_hbm *nodeBank2,
    hls::split::load_balance<unit_interval, 2, 20> &rngStream);

#endif
