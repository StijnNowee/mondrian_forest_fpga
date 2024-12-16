#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"

void top_lvl(
    feature_vector *inputFeature,
    Page pageBank1[MAX_PAGES]
);

void streamMerger(hls::stream<FetchRequest> &newRequest, FetchRequest &feedbackRegister, hls::stream<FetchRequest> &fetchRequest);


#endif /* TOP_LVL_H_ */