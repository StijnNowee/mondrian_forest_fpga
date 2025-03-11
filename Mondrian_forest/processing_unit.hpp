#ifndef PROCESSING_HPP
#define PROCESSING_HPP
#include "common.hpp"

void processing_unit(hls::stream<input_t> &inputFeatureStream, Page *pageBank, const int trainingSize, const int totalSize, hls::stream<Result> &inferenceOutputStream);

#endif