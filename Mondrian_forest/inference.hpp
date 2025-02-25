#ifndef INFERENCE_H
#define INFERENCE_H

#include "common.hpp"

// void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream, hls::stream<Result> &resultOutputStream, const Page *pagePool);
void inference(hls::stream<input_vector> &inferenceInputStream, hls::stream<int> &processTreeStream, hls::stream<Result> &resultOutputStream, const Page *pagePool);
void condenser(hls::stream<int> &processTreeStream ,const Page *pagePool, Node_sml trees[TREES_PER_BANK][MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE]);

#endif