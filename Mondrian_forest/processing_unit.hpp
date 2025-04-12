#ifndef PROCESSING_HPP
#define PROCESSING_HPP
#include "common.hpp"
#include <hls_stream.h>
#include <hls_streamofblocks.h>

void processing_unit(hls::stream<input_vector> &trainInputStream, hls::stream<input_vector> &inferenceInputStream, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], Page *trainPageBank, const Page *inferencePageBank, const InputSizes &sizes, hls::stream<ClassDistribution> inferenceOutputStream[TREES_PER_BANK], int maxPageNr[TREES_PER_BANK]);

template <int TRAVERSAL_BLOCKS, typename T, typename P>
void fetcher(hls::stream<T> &fetchRequestStream, hls::stream_of_blocks<IPage> pageOutS[TRAVERSAL_BLOCKS], const Page *pageBank);
void update_weight(Node_hbm &node);

#endif