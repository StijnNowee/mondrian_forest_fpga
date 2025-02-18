#include "top_lvl.hpp"
#include "train.hpp"
#include "rng.hpp"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page *pageBank1,
    const int size
)  {
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    #pragma HLS INTERFACE axis port=inputFeatureStream depth=10
    

    train(inputFeatureStream, pageBank1, size);


    
}