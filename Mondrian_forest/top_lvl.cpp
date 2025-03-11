#include "top_lvl.hpp"
#include "processing_unit.hpp"
#include <hls_task.h>

void top_lvl(
    hls::stream<input_t> &trainInputStream1,
    hls::stream<input_t>  &inferenceInputStream1,
    hls::stream<Result> &inferenceOutputStream1,
    hls::stream<input_t> &trainInputStream2,
    hls::stream<input_t>  &inferenceInputStream2,
    hls::stream<Result> &inferenceOutputStream2,
    const int size,
    //Page *pageBank1,
    Page *pageBank1,
    Page *pageBank2
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_none port=size
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    #pragma HLS INTERFACE m_axi port=pageBank2 bundle=hbm1 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    #pragma HLS INTERFACE ap_ctrl_chain port=return

    processing_unit(trainInputStream2, pageBank2, size, inferenceInputStream2, inferenceOutputStream2);
    processing_unit(trainInputStream1, pageBank1, size, inferenceInputStream1, inferenceOutputStream1);
    
    
}

void convertInputToVector(const input_t &raw, input_vector &input){
    *reinterpret_cast<input_t*>(&input) = raw;
}