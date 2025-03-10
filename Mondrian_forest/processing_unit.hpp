#ifndef PROCESSING_HPP
#define PROCESSING_HPP
#include "common.hpp"

void processing_unit(hls::stream<input_t> &inputFeatureStream,Page *pageBank1, const int size, hls::stream<input_t> &inferenceInputStream, hls::stream<Result> &inferenceOutputStream);

#endif