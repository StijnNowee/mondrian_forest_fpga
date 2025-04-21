#include "rng.hpp"

void rng_generator(hls::stream<unit_interval, 20> rngStream[BANK_COUNT][TRAIN_TRAVERSAL_BLOCKS])
{
    #ifdef CALIBRATION
    constexpr int multiplier = 10;
    #else
    constexpr int multiplier = 100;
    #endif
    static ap_uint<8> lfsr_state = 0x42;
    unit_interval rand_val;
    for(int i = 0; i < multiplier*TREES_PER_BANK; i++){
        for(int b = 0; b < BANK_COUNT; b++){
            for(int t = 0; t < TRAIN_TRAVERSAL_BLOCKS; t++){
                #pragma HLS PIPELINE II=2
                if(!rngStream[b][t].full()){
                    bool feedback_bit = lfsr_state[7] ^ lfsr_state[6] ^ lfsr_state[5] ^ lfsr_state[4];
                    lfsr_state = (lfsr_state << 1) | feedback_bit;
                    rand_val.setBits(lfsr_state);
                    rngStream[b][t].write(rand_val);
                }
            }

        }
    }
}
