#include "control_unit.hpp"

void control_unit(feature_vector *newFeature,
    hls::stream<feature_vector> &fetchRequestStream)
{
    #pragma HLS INTERFACE mode=s_axilite port=newFeature

    auto feature = *newFeature;
    fetchRequestStream.write(feature);

}
