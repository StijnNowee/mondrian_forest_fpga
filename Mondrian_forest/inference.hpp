#ifndef INFERENCE_H
#define INFERENCE_H

#include "common.hpp"

// void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream, hls::stream<Result> &resultOutputStream, const Page *pagePool);
void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<int> &processTreeStream, hls::stream<Result> &resultOutputStream, const Page *pagePool);

#endif