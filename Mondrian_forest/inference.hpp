#ifndef INFERENCE_H
#define INFERENCE_H

#include "common.hpp"
#include <hls_streamofblocks.h>

// void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream, hls::stream<Result> &resultOutputStream, const Page *pagePool);
void inference(hls::stream<input_t> &inferenceInputStream, hls::stream<ap_uint<50>> &inferenceOutputStream, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<bool> &treeUpdateCtrlStream);
//void condenser(hls::stream<int> &processTreeStream ,const Page *pagePool, hls::stream_of_blocks<trees_t> &treeStream);

#endif