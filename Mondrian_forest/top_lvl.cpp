#include "top_lvl.hpp"
#include "train.hpp"
#include "rng.hpp"


void top_lvl(
    hls::stream<input_t> &inputFeatureStream,
    hls::stream<int> &outputStream,
    Page *pageBank1
)  {
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK offset=slave
    #pragma HLS stable variable=pageBank1
    
    train(inputFeatureStream, outputStream, pageBank1);
}