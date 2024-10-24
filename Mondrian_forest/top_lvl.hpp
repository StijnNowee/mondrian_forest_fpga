#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

extern "C" {
    static Node_hbm nodePool[MAX_NODES];
    static Tree treePool[TREE_COUNT];
    void top_lvl(hls::stream<feature_vector> &inputFeatureStream, hls::stream<label_vector> &inputLabelStream, hls::stream<label_vector> &outputStream);
}

#endif /* TOP_LVL_H_ */