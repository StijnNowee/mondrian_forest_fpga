#include "control_unit.hpp"
#include "training.hpp"

void control_unit(hls::stream<feature_vector> &inputFeatureStream,
    hls::stream<label_vector> &inputLabelStream,
    Tree *treePool,
    Node_hbm *nodeBank1,
    Node_hbm *nodeBank2,
    hls::split::load_balance<unit_interval, 2, 20> &rngStream)
{
    #pragma HLS DATAFLOW

}