#include "rng.hpp"

void rng_generator(hls::stream<unit_interval, 20> rngStream[BANK_COUNT][TRAIN_TRAVERSAL_BLOCKS], hls::stream<bool> doneStream[2])
{
    static ap_uint<8> lfsr_state = 0x42;
    unit_interval rand_val;
    for(int i = 0; i < 2; i++){
        #ifdef __SYNTHESIS__
        while(doneStream[i].empty())
        #else
        for(int i = 0; i < 10000; i++)
        #endif
        {
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
        doneStream[i].read();
    }
}
