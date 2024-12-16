#ifndef CONTROL_UNIT_H_
#define CONTROL_UNIT_H_
#include "common.hpp"
void control_unit(feature_vector *newFeature,
    hls::stream<feature_vector> &fetchRequestStream);

#endif
