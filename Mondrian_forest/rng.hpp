#ifndef RNG_HPP
#define RNG_HPP

#include "common.hpp"

void rng_generator(hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS], bool &done);
#endif