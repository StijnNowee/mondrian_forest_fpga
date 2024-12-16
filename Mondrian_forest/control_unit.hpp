#ifndef CONTROL_UNIT_H_
#define CONTROL_UNIT_H_
#include "common.hpp"
void control_unit(feature_vector *newFeature,
    Page *pageBank1_read,
    hls::stream<FetchRequest> &fetchRequestStream);

#endif
