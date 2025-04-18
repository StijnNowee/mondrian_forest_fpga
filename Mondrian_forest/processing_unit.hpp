#ifndef PROCESSING_HPP
#define PROCESSING_HPP
#include "common.hpp"
#include <hls_stream.h>
#include <hls_streamofblocks.h>

void processing_unit(hls::stream<input_vector> &trainInputStream, hls::stream<input_vector> &inferenceInputStream, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], Page trainPageBank[TREES_PER_BANK*MAX_PAGES_PER_TREE], const Page inferencePageBank[TREES_PER_BANK*MAX_PAGES_PER_TREE], const int sizes[2], hls::stream<ClassSums> &inferenceOutputStream, int maxPageNr[TREES_PER_BANK], hls::stream<int> &executionCountStream);

template <int TRAVERSAL_BLOCKS, typename T, typename P>
void fetcher(hls::stream<T> &fetchRequestStream, hls::stream_of_blocks<IPage> pageOutS[TRAVERSAL_BLOCKS], const Page *pageBank, const int &blockIdx);
void update_weight(Node_hbm &node);

#endif