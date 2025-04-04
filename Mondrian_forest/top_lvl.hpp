#ifndef TOP_LVL_H_
#define TOP_LVL_H_

#include "common.hpp"
#include <hls_stream.h>

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &inferenceOutputStream,
    const InputSizes &sizes,
    PageBank hbmMemory[BANK_COUNT]
    //PageBank pageBank1, PageBank pageBank2, PageBank pageBank3, PageBank pageBank4, PageBank pageBank5//, PageBank pageBank6, PageBank pageBank7, PageBank pageBank8,PageBank pageBank9, PageBank pageBank10,
    //PageBank pageBank11, PageBank pageBank12, PageBank pageBank13, PageBank pageBank14, PageBank pageBank15, PageBank pageBank16, PageBank pageBank17, PageBank pageBank18,PageBank pageBank19, PageBank pageBank20
);

#endif /* TOP_LVL_H_ */