#include "rng.hpp"

void generate_rng(hls::stream<unit_interval> &rngStream1, hls::stream<unit_interval> &rngStream2)
{
    ap_uint<8> lfsr = 0xAB;
    ap_uint<1> dout;
    // #ifdef __SYNTHESIS__
    // while(true) {
    //     #pragma HLS PIPELINE II=1
    //     if(!rngStream.full()){
    //         std::cout << "generated ?" << std::endl;
            
    //         bool lsb = lfsr.get_bit(0);
    //         lfsr >>= 1;
    //         if (lsb) {
    //             lfsr ^= (ap_uint<8>)0xB8;
    //         }
    //         ap_ufixed<8,0> random_number = lfsr;

    //         rngStream.write(random_number);
    //     }
    // }
    // #else
    for(int i =0; i < 100; i++) {
        #pragma HLS PIPELINE II=1
        if(!rngStream1.full()){
            
            bool lsb = lfsr.get_bit(0);
            lfsr >>= 1;
            if (lsb) {
                lfsr ^= (ap_uint<8>)0xB8;
            }
            ap_ufixed<8,0> random_number = lfsr;

            rngStream1.write(random_number);
        }
        if(!rngStream2.full()){
            
            bool lsb = lfsr.get_bit(0);
            lfsr >>= 1;
            if (lsb) {
                lfsr ^= (ap_uint<8>)0xB8;
            }
            ap_ufixed<8,0> random_number = lfsr;

            rngStream2.write(random_number);
        }
    }
   // #endif
}