#include "top_lvl.hpp"
#include "processing_unit.hpp"
#include <hls_task.h>
void inputSplitter(hls::stream<input_t> &inputStream, hls::stream<input_t> splitInputStreams[BANK_COUNT], const int totalSize);

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &inferenceOutputStream1,
    hls::stream<Result> &inferenceOutputStream2,
    const int totalSize,
    const int trainSize,
    Page *pageBank1,
    Page *pageBank2
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_none port=totalSize
    #pragma HLS INTERFACE ap_none port=trainSize
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    #pragma HLS INTERFACE m_axi port=pageBank2 bundle=hbm1 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    #pragma HLS INTERFACE ap_ctrl_chain port=return

    hls::stream<input_t> splitInputStreams[BANK_COUNT];

    inputSplitter(inputStream, splitInputStreams, totalSize);

    processing_unit(splitInputStreams[0], pageBank1, trainSize, totalSize, inferenceOutputStream1);
    processing_unit(splitInputStreams[1], pageBank2, trainSize, totalSize, inferenceOutputStream2);
    
}

void convertInputToVector(const input_t &raw, input_vector &input){
    *reinterpret_cast<input_t*>(&input) = raw;
}

void inputSplitter(hls::stream<input_t> &inputStream, hls::stream<input_t> splitInputStreams[BANK_COUNT], const int totalSize)
{
    for(int i = 0; i < totalSize; i++){
        auto input = inputStream.read();
        for(int b = 0; b < BANK_COUNT; b++){
            splitInputStreams[b].write(input);
        }
    }
}