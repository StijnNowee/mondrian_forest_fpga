#ifndef INFERENCE_H
#define INFERENCE_H

#include "common.hpp"
#include <hls_streamofblocks.h>

// void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream, hls::stream<Result> &resultOutputStream, const Page *pagePool);
void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<Result> &resultOutputStream, hls::stream_of_blocks<trees_t> &treeStream);
//void condenser(hls::stream<int> &processTreeStream ,const Page *pagePool, hls::stream_of_blocks<trees_t> &treeStream);

#endif