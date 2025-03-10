#include "top_lvl.hpp"
#include "processing_unit.hpp"
#include <hls_task.h>

void top_lvl(
    hls::stream<input_t> &trainInputStream,
    hls::stream<input_t>  &inferenceInputStream,
    hls::stream<Result> &inferenceOutputStream,
    const int size,
    //Page *pageBank1,
    Page *pageBank1
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_none port=size
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    #pragma HLS INTERFACE ap_ctrl_chain port=return

    processing_unit(trainInputStream, pageBank1, size, inferenceInputStream, inferenceOutputStream);

    
}

void convertInputToVector(const input_t &raw, input_vector &input){
    *reinterpret_cast<input_t*>(&input) = raw;
}