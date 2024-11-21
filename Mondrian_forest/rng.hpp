
#ifndef RNG_H_
#define RNG_H_

#include "common.hpp"
#include <cstdint>
void generate_rng(hls::stream<unit_interval> &rngStream1, hls::stream<unit_interval> &rngStream2);

#endif