#ifndef INFERENCE_H
#define INFERENCE_H

#include "common.hpp"
#include <hls_stream.h>

void inference(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<Feedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput, const PageBank &pageBank);

#endif