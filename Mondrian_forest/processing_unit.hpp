#ifndef PROCESSING_HPP
#define PROCESSING_HPP
#include "common.hpp"
#include <hls_stream.h>
#include <hls_streamofblocks.h>

void processing_unit(hls::stream<input_t> &inputFeatureStream, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], PageBank &pageBank, const PageBank &readOnlyPageBank, const InputSizes &sizes, hls::stream<ClassDistribution> &inferenceOutputStream);

template <int TRAVERSAL_BLOCKS>
void fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> pageOutS[TRAVERSAL_BLOCKS], const PageBank &pageBank);

#endif