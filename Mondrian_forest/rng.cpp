#include "rng.hpp"

void generate_number(ap_uint<8> &lfsr_state, unit_interval &value);

void rng_generator(hls::stream<unit_interval> rngStream[2*BANK_COUNT])
{
    #pragma HLS inline off
    
    ap_uint<8> lfsr_state = 0x01;
    unit_interval rand_val;
    for(int i =0; i < 1000; i++){
        for(int j = 0; j < 2*BANK_COUNT; j++){
            if(!rngStream[j].full()){
                generate_number(lfsr_state, rand_val);
                rngStream[j].write(rand_val);
            }
        }
    }
}

void generate_number(ap_uint<8> &lfsr_state, unit_interval &value)
{
    bool lsb = lfsr_state & 0x1;
    lfsr_state >>= 1;
    if(lsb){
        lfsr_state ^= ap_uint<8>(0x8E);
    }
    value.setBits(lfsr_state);
}