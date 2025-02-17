#include "rng.hpp"

void rng_generator(hls::stream<unit_interval> rngStream[2*BANK_COUNT])
{
    static ap_uint<8> lfsr_state = 0x42;
    unit_interval rand_val;
    //for(int i =0; i < 1000; i++){
        for(int j = 0; j < 2*BANK_COUNT; j++){
            #pragma HLS UNROLL off
            if(!rngStream[j].full()){

                bool feedback_bit = lfsr_state[7] ^ lfsr_state[6] ^ lfsr_state[5] ^ lfsr_state[4];

                lfsr_state = (lfsr_state << 1) | feedback_bit;
                rand_val.setBits(lfsr_state);
                rngStream[j].write(rand_val);
            }
        }
    //}
}
