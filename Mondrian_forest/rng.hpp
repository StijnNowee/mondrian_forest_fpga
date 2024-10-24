
#ifndef RNG_H_
#define RNG_H_

#include "common.hpp"
#include <cstdint>
extern "C" {
void generate_rng(hls::stream<fixed_point> &rngStream)
{
    #pragma HLS INLINE off
    #pragma HLS STREAM depth=16 variable=rngStream

    const unsigned int a = 1664525;
    const unsigned int c = 1013904223;
    unsigned int seed = 42;

    while(true){
        #pragma HLS PIPELINE II=1
        seed = a * seed + c;
        fixed_point rng_value = static_cast<fixed_point>(seed)/ static_cast<fixed_point>(UINT32_MAX);
        rngStream.write(rng_value);
    }
}
}

#endif