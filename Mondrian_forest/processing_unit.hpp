#ifndef PROCESSING_HPP
#define PROCESSING_HPP
#include "common.hpp"

void processing_unit(hls::stream<input_t> &inputFeatureStream, hls::stream<unit_interval> &rngStream, Page *pageBank, const InputSizes &sizes, hls::stream<ClassDistribution> &inferenceOutputStream);

#endif