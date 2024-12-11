#ifndef CONTROL_UNIT_H_
#define CONTROL_UNIT_H_
#include "common.hpp"
#include "hls_np_channel.h"
void control_unit(
    hls::stream<feature_vector> &inputFeatureStream,
    Page *pageBank1_read,
    Page *pageBank2_read,
    hls::split::load_balance<unit_interval, 4, 20> &rngStream
    );

    struct FetchRequest{
        feature_vector feature;
        int pageIdx;
    };

    void streamMerger(hls::stream<feature_vector> &newFeatureStream, hls::stream<FetchRequest> &requestStream);
    void featureDistributer(hls::stream<feature_vector> &inputFeatureStream, hls::stream<feature_vector> &newFeatureStream);
#endif
