#include "rng.hpp"

void rng_generator(hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS])
{
    ap_uint<8> lfsr_state = 0x42;
    unit_interval rand_val;
    #ifdef __SYNTHESIS__
    while(true){
    #else
    for(int i = 0; i < 1000000; i++){
    #endif
        for(int b = 0; b < BANK_COUNT*TRAVERSAL_BLOCKS; b++){
            #pragma HLS PIPELINE II=4
            if(!rngStream[b].full()){
                bool feedback_bit = lfsr_state[7] ^ lfsr_state[6] ^ lfsr_state[5] ^ lfsr_state[4];
                lfsr_state = (lfsr_state << 1) | feedback_bit;
                rand_val.setBits(lfsr_state);
                rngStream[b].write(rand_val);
            }
        }
    }
}
