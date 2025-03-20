#include "top_lvl.hpp"
#include "processing_unit.hpp"
#include "rng.hpp"
#include <hls_np_channel.h>

void inputSplitter(hls::stream<input_t> &inputStream, hls::stream<input_t> splitInputStreams[BANK_COUNT], const int totalSize);
void total_voter(hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &inferenceOuputStream, const int size);

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &inferenceOutputStream,
    const InputSizes &sizes,
    PageBank pageBank1, PageBank pageBank2, PageBank pageBank3, PageBank pageBank4, PageBank pageBank5, PageBank pageBank6, PageBank pageBank7, PageBank pageBank8,PageBank pageBank9, PageBank pageBank10,
    PageBank pageBank11, PageBank pageBank12, PageBank pageBank13, PageBank pageBank14, PageBank pageBank15, PageBank pageBank16, PageBank pageBank17, PageBank pageBank18,PageBank pageBank19, PageBank pageBank20
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_none port=sizes
    #pragma HLS INTERFACE m_axi port=pageBank1 bundle=hbm1 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank2 bundle=hbm2 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank3 bundle=hbm3 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank4 bundle=hbm4 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank5 bundle=hbm5 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank6 bundle=hbm6 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank7 bundle=hbm7 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank8 bundle=hbm8 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank9 bundle=hbm9 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank10 bundle=hbm10 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank11 bundle=hbm11 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank12 bundle=hbm12 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank13 bundle=hbm13 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank14 bundle=hbm14 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank15 bundle=hbm15 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank16 bundle=hbm16 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank17 bundle=hbm17 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank18 bundle=hbm18 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank19 bundle=hbm19 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBank20 bundle=hbm20 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBanks[0] bundle=hbm0 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE m_axi port=pageBanks[1] bundle=hbm1 depth=MAX_PAGES_PER_TREE*TREES_PER_BANK
    // #pragma HLS INTERFACE ap_ctrl_chain port=return

    //hls::split::load_balance<unit_interval, 2, 10> rngStream("rngStream");
    hls::stream<input_t> splitInputStreams[BANK_COUNT];
    hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT];
    hls::stream<unit_interval> rngStream[BANK_COUNT*TRAVERSAL_BLOCKS];

    rng_generator(rngStream);
    inputSplitter(inputStream, splitInputStreams, sizes.total);
    
    //hls::task rngTask(rng_generator, rngStream.in);
    processing_unit(splitInputStreams[0], rngStream, pageBank1, sizes, splitInferenceOutputStreams[0], 0);
    // processing_unit(splitInputStreams[1], rngStream, pageBank2, sizes, splitInferenceOutputStreams[1], 1);
    // processing_unit(splitInputStreams[2], rngStream, pageBank3, sizes, splitInferenceOutputStreams[2], 2);
    // processing_unit(splitInputStreams[3], rngStream, pageBank4, sizes, splitInferenceOutputStreams[3], 3);
    // processing_unit(splitInputStreams[4], rngStream, pageBank5, sizes, splitInferenceOutputStreams[4], 4);
    // processing_unit(splitInputStreams[5], rngStream, pageBank6, sizes, splitInferenceOutputStreams[5], 5);
    // processing_unit(splitInputStreams[6], rngStream, pageBank7, sizes, splitInferenceOutputStreams[6], 6);
    // processing_unit(splitInputStreams[7], rngStream, pageBank8, sizes, splitInferenceOutputStreams[7], 7);
    // processing_unit(splitInputStreams[8], rngStream, pageBank9, sizes, splitInferenceOutputStreams[8], 8);
    // processing_unit(splitInputStreams[9], rngStream, pageBank10, sizes, splitInferenceOutputStreams[9], 9);
    // processing_unit(splitInputStreams[10], rngStream, pageBank11, sizes, splitInferenceOutputStreams[10], 10);
    // processing_unit(splitInputStreams[11], rngStream, pageBank12, sizes, splitInferenceOutputStreams[11], 11);
    // processing_unit(splitInputStreams[12], rngStream, pageBank13, sizes, splitInferenceOutputStreams[12], 12);
    // processing_unit(splitInputStreams[13], rngStream, pageBank14, sizes, splitInferenceOutputStreams[13], 13);
    // processing_unit(splitInputStreams[14], rngStream, pageBank15, sizes, splitInferenceOutputStreams[14], 14);
    // processing_unit(splitInputStreams[15], rngStream, pageBank16, sizes, splitInferenceOutputStreams[15], 15);
    // processing_unit(splitInputStreams[16], rngStream, pageBank17, sizes, splitInferenceOutputStreams[16], 16);
    // processing_unit(splitInputStreams[17], rngStream, pageBank18, sizes, splitInferenceOutputStreams[17], 17);
    // processing_unit(splitInputStreams[18], rngStream, pageBank19, sizes, splitInferenceOutputStreams[18], 18);
    // processing_unit(splitInputStreams[19], rngStream, pageBank20, sizes, splitInferenceOutputStreams[19], 19);
    total_voter(splitInferenceOutputStreams, inferenceOutputStream, sizes.inference);

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

void total_voter(hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &inferenceOuputStream, const int size)
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
}