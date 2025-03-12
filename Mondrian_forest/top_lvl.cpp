#include "top_lvl.hpp"
#include "processing_unit.hpp"
#include "rng.hpp"
#include <hls_np_channel.h>

void inputSplitter(hls::stream<input_t> &inputStream, hls::stream<input_t> splitInputStreams[BANK_COUNT], const int totalSize);
void total_voter(hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &inferenceOuputStream, const int size, bool &done);

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &inferenceOutputStream,
    const InputSizes &sizes,
    Page *pageBank1,
    Page *pageBank2
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_none port=sizes
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    #pragma HLS INTERFACE m_axi port=pageBank2 bundle=hbm1 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE ap_ctrl_chain port=return

    //hls::split::load_balance<unit_interval, 2, 10> rngStream("rngStream");
    hls::stream<input_t> splitInputStreams[BANK_COUNT];
    hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT];
    hls::stream<unit_interval> rngStream[BANK_COUNT];
    bool done = false;

    rng_generator(rngStream, done);
    inputSplitter(inputStream, splitInputStreams, sizes.total);
    
    //hls::task rngTask(rng_generator, rngStream.in);
    processing_unit(splitInputStreams[0], rngStream[0], pageBank1, sizes, splitInferenceOutputStreams[0]);
    processing_unit(splitInputStreams[1], rngStream[1], pageBank2, sizes, splitInferenceOutputStreams[1]);
    //processing_unit(splitInputStreams[1], rngStream.out[1], pageBank2, sizes, splitInferenceOutputStreams[1]);
    total_voter(splitInferenceOutputStreams, inferenceOutputStream, sizes.inference, done);

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

void total_voter(hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &inferenceOuputStream, const int size, bool &done)
{
    ClassDistribution results[BANK_COUNT];
    for (int i = 0; i < size; i++) {
        ap_ufixed<24, 16> classSums[CLASS_COUNT] = {0};
        for(int b = 0; b < BANK_COUNT; b++){
            results[b] = splitInferenceOutputStreams[b].read();
            for(int c = 0; c < CLASS_COUNT; c++){
                classSums[c] += results[b].distribution[c];
            }
        }

        ClassDistribution avg;
        for(int c = 0; c < CLASS_COUNT; c++){
            avg.distribution[c] = classSums[c]/BANK_COUNT;
        }
        
        //TODO change to reduction tree
        Result finalResult{0, avg.distribution[0]};
        // ap_ufixed<9,1> max_confidence = avg.distribution[0];
        // int resultClass = 0;
        for(int c = 1; c < CLASS_COUNT; c++){
            if(avg.distribution[c] > finalResult.confidence){
                finalResult.confidence = avg.distribution[c];
                finalResult.resultClass = c;
            }
        }
        inferenceOuputStream.write(finalResult);

    }
    done = true;
}