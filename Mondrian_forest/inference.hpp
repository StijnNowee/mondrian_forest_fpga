#ifndef INFERENCE_H
#define INFERENCE_H

#include "common.hpp"
#include <hls_stream.h>

void inference(hls::stream<IFetchRequest> &fetchRequestStream, hls::stream<IFeedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput, const PageBank &pageBank);

#endif