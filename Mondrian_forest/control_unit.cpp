#include "control_unit.hpp"

void control_unit(feature_vector *newFeature,
    Page *pageBank1_read,
    hls::stream<FetchRequest> &fetchRequestStream)
{
    #pragma HLS INTERFACE mode=s_axilite port=newFeature

    auto feature = *newFeature;
    fetchRequestStream.write(FetchRequest{.feature = feature, .pageIdx = 0});

}
