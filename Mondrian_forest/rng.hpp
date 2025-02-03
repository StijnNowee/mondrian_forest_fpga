#ifndef RNG_HPP
#define RNG_HPP

#include "common.hpp"

void rng_generator(hls::stream<unit_interval> rngStream[2]);
#endif