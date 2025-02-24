#include "top_lvl.hpp"
#include "train.hpp"
#include "inference.hpp"
#include <hls_task.h>

void it_switch(hls::stream<input_t> &inputStream, hls::stream<input_vector> &trainInputStream, hls::stream<input_vector> &inferenceInputStream);

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<node_t> &outputStream,
    hls::stream<Result> &resultOutputStream,
    Page *pageBank1
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK offset=slave
    #pragma HLS stable variable=pageBank1
    
    //hls_thread_local hls::stream<int> processDoneStream("ProcessDoneStream");
    hls_thread_local hls::stream<int> processTreeStream("ProcessTreeStream");
    hls_thread_local hls::stream<input_vector> trainInputStream("TrainInputStream");
    hls_thread_local hls::stream<input_vector> inferenceInputStream("InferenceInputStream");

    hls_thread_local hls::task t1(it_switch, inputStream, trainInputStream, inferenceInputStream);
    //train(trainInputStream, processTreeStream, processDoneStream, outputStream, pageBank1);
    train(trainInputStream, processTreeStream, outputStream, pageBank1);

    //inference(inferenceInputStream, processTreeStream, processDoneStream, resultOutputStream, pageBank2);
    inference(inferenceInputStream, processTreeStream, resultOutputStream, pageBank1);
}

void it_switch(hls::stream<input_t> &inputStream, hls::stream<input_vector> &trainInputStream, hls::stream<input_vector> &inferenceInputStream)
{
    auto rawInput = inputStream.read();
    input_vector newInput;
    newInput.label = rawInput.range(CLASS_BITS, 1);
    for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
        newInput.feature[i].range(7,0) = rawInput.range(CLASS_BITS + 8*(i+1), CLASS_BITS + 1 + 8*i);
    }
    if(rawInput.get_bit(0)){
        trainInputStream.write(newInput);
    }else{
        inferenceInputStream.write(newInput);
    }

}