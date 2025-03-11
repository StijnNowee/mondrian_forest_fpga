#ifndef INFERENCE_H
#define INFERENCE_H

#include "common.hpp"
#include <hls_streamofblocks.h>

// void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream, hls::stream<Result> &resultOutputStream, const Page *pagePool);
void run_inference(hls::stream<input_t> &inferenceStream, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<Result> &inferenceOutputStreams,  hls::stream<bool> &treeUpdateCtrlStream);
void inference(hls::stream<input_vector> &inferenceStream, hls::stream<Result> &inferenceOutputStream, const trees_t &smlTreeBank, const int size);
//void condenser(hls::stream<int> &processTreeStream ,const Page *pagePool, hls::stream_of_blocks<trees_t> &treeStream);

#endif