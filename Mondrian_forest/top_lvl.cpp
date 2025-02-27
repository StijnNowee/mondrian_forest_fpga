#include "top_lvl.hpp"
#include "train.hpp"
#include "inference.hpp"
#include <hls_task.h>

void it_switch(hls::stream<input_t> &inputStream, hls::stream<input_vector> &trainInputStream, hls::stream<input_vector> &inferenceInputStream, const int size);

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<node_t> &outputStream,
    hls::stream<bool> &controlOutputStream,
    hls::stream<Result> &resultOutputStream,
    //Page *pageBank1,
    Page *pageBank1,
    const int size
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_ctrl_none port=return
    //#pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK offset=slave
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK offset=slave
    //#pragma HLS stable variable=pageBank1
    #pragma HLS stable variable=pageBank1
    
    hls_thread_local hls::stream_of_blocks<trees_t, 2> treeStream;

    //hls::stream<input_t> trainInputStream("TrainInputStream");
    hls::stream<input_vector> inferenceInputStream("InferenceInputStream");
    hls::stream<input_vector> trainInputStream("TrainInputStream");

    
    train(trainInputStream, outputStream, controlOutputStream, pageBank1, treeStream);

    //inference(inferenceInputStream, resultOutputStream, treeStream);
    it_switch(inputStream, trainInputStream, inferenceInputStream, size);
}

void convertInputToVector(const input_t &raw, input_vector &input){
    *reinterpret_cast<input_t*>(&input) = raw;
}

void it_switch(hls::stream<input_t> &inputStream, hls::stream<input_vector> &trainInputStream, hls::stream<input_vector> &inferenceInputStream, const int size)//, hls::stream<int> &processTreeStream)
{
    #pragma HLS inline off
    for(int i = 0; i < size; i++){
        input_t rawInput = inputStream.read();
        input_vector input;
        convertInputToVector(rawInput, input);
        if(input.trainSample){
            trainInputStream.write(input);
        //}else{
            //inferenceInputStream.write(input);
        }
    }
}