#include "rng.hpp"

void rng_generator(hls::stream<unit_interval> &rngStream)
{
    static ap_uint<8> lfsr_state = 0x42;
    unit_interval rand_val;

    bool feedback_bit = lfsr_state[7] ^ lfsr_state[6] ^ lfsr_state[5] ^ lfsr_state[4];

    lfsr_state = (lfsr_state << 1) | feedback_bit;
    rand_val.setBits(lfsr_state);
    rngStream.write(rand_val);

}
