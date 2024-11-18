
#ifndef RNG_H_
#define RNG_H_

#include "common.hpp"
#include <cstdint>
extern "C" {
void generate_rng(hls::stream<unit_interval> &rngStream);
}

#endif