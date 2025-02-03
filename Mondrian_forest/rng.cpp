#include "rng.hpp"

void rng_generator(hls::stream<unit_interval> rngStream[2])
{
    #pragma HLS inline off
    
        ap_uint<8> lfsr_state = 0x01;
        unit_interval rand_val;
    for(int i =0; i < 1000; i++){
        bool lsb = lfsr_state & 0x1;
        lfsr_state >>= 1;
        if(lsb){
            lfsr_state ^= ap_uint<8>(0x8E);
        }

        rand_val.setBits(lfsr_state);
        rngStream[0].write(rand_val);
        rngStream[1].write(rand_val);
    }
}