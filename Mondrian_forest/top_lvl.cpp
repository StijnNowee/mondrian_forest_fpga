#include "top_lvl.hpp"
#include "train.hpp"
#include "inference.hpp"
#include <hls_task.h>

void it_switch(hls::stream<input_t> &inputStream, hls::stream<input_t> &trainInputStream);
void top_lvl(
    hls::stream<input_t> &trainInputStream,
    hls::stream<input_t>  &inferenceInputStream,
    hls::stream<ap_uint<50>> &inferenceOutputStream,
    const int size,
    //Page *pageBank1,
    Page *pageBank1
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_none port=size
    //#pragma HLS INTERFACE axis port=trainInputStream
    // #pragma HLS INTERFACE axis port=inferenceInputStream
    // #pragma HLS INTERFACE axis port=inferenceOutputStream
    //#pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK offset=slave
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    //#pragma HLS stable variable=pageBank1
    #pragma HLS INTERFACE ap_ctrl_chain port=return
    //#pragma HLS stable variable=pageBank1
    //#pragma HLS INTERFACE ap_ctrl_none port=return
    
    // hls_thread_local hls::stream_of_blocks<trees_t, 3> treeStream;
    // hls_thread_local hls::stream<bool> treeUpdateCtrlStream("TreeUpdateCtrlStream");

    //hls::stream<input_t> trainInputStream("TrainInputStream");
    // hls_thread_local hls::stream<input_vector> inferenceInputStream("InferenceInputStream");
    
   // hls::task t1(it_switch, inputStream, trainInputStream);
    train(trainInputStream, pageBank1, size, inferenceInputStream, inferenceOutputStream);

    //inference(inferenceInputStream, inferenceOutputStream, treeStream, treeUpdateCtrlStream);
    
}

void convertInputToVector(const input_t &raw, input_vector &input){
    *reinterpret_cast<input_t*>(&input) = raw;
}

// void it_switch(hls::stream<input_t> &inputStream, hls::stream<input_t> &trainInputStream, hls::stream<input_t> &trainInputStream)//, hls::stream<int> &processTreeStream)
// {
//     auto rawInput = inputStream.read();
//     //input_vector newInput;
//     // newInput.label = rawInput.range(CLASS_BITS, 1);
//     // for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
//     //     newInput.feature[i].range(7,0) = rawInput.range(CLASS_BITS + 8*(i+1), CLASS_BITS + 1 + 8*i);
//     // }
//     trainInputStream.write(rawInput);
//     // if(true){ //rawInput.get_bit(0)
//     //     //processTreeStream.write(0);
        
//     // }else{
//     //      inferenceInputStream.write(newInput);
//     // }

// }