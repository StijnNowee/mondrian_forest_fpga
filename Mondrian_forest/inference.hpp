#ifndef INFERENCE_H
#define INFERENCE_H

#include "common.hpp"
#include <hls_stream.h>

void inference(hls::stream<IFetchRequest> &fetchRequestStream, hls::stream<IFeedback> &feedbackStream, hls::stream<ClassDistribution> voterOutput[TREES_PER_BANK], const Page *pageBank, const int &blockIdx);

#endif