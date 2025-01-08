#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"
#include "hls_np_channel.h"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page pageBank1[MAX_PAGES],
    hls::split::load_balance<unit_interval, 2, 20> &rngStream
);

void streamMerger(hls::stream<FetchRequest> &newRequest, FetchRequest &feedbackRegister, hls::stream<FetchRequest> &fetchRequest);


#endif /* TOP_LVL_H_ */