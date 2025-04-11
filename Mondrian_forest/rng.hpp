#ifndef RNG_HPP
#define RNG_HPP

#include "common.hpp"
#include <hls_stream.h>

void rng_generator(hls::stream<unit_interval, 20> rngStream[BANK_COUNT][TRAIN_TRAVERSAL_BLOCKS], hls::stream<bool> doneStream[2]);
#endif