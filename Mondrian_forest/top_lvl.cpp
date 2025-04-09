#include "top_lvl.hpp"
#include "processing_unit.hpp"
#include "rng.hpp"

void inputSplitter(hls::stream<input_t> &inputStream, hls::stream<input_t> splitInputStreams[BANK_COUNT], const int totalSize);
void total_voter(hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &inferenceOuputStream, const int size);

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &inferenceOutputStream,
    const InputSizes &sizes,
    PageBank hbmMemory[BANK_COUNT]
    //PageBank pageBank1, PageBank pageBank2, PageBank pageBank3, PageBank pageBank4, PageBank pageBank5//, PageBank pageBank6, PageBank pageBank7, PageBank pageBank8,PageBank pageBank9, PageBank pageBank10,
    //PageBank pageBank11, PageBank pageBank12, PageBank pageBank13, PageBank pageBank14, PageBank pageBank15, PageBank pageBank16, PageBank pageBank17, PageBank pageBank18,PageBank pageBank19, PageBank pageBank20
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_none port=sizes
    //#pragma HLS INTERFACE m_axi port=hbmMemory depth=BANK_COUNT bundle=hbm
    #pragma HLS ARRAY_PARTITION variable=hbmMemory dim=1 type=complete

    //hls::split::load_balance<unit_interval, 2, 10> rngStream("rngStream");
    hls::stream<input_t> splitInputStreams[BANK_COUNT];
    hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT];
    hls::stream<unit_interval, 20> rngStream[BANK_COUNT][TRAIN_TRAVERSAL_BLOCKS];
    //hls::split::load_balance<unit_interval, BANK_COUNT> rngStream;

    rng_generator(rngStream);
    inputSplitter(inputStream, splitInputStreams, sizes.total);
    
    //hls::task rngTask(rng_generator, rngStream.in);
    for(int b = 0; b < BANK_COUNT; b++){
        #pragma HLS UNROLL
        processing_unit(splitInputStreams[b], rngStream[b], hbmMemory[b], sizes, splitInferenceOutputStreams[b]);
    }
    //total_voter(splitInferenceOutputStreams, inferenceOutputStream, sizes.inference);

}

void inputSplitter(hls::stream<input_t> &inputStream, hls::stream<input_t> splitInputStreams[BANK_COUNT], const int totalSize)
{
    for(int i = 0; i < totalSize; i++){
        auto input = inputStream.read();
        std::cout << "Input: " << i << std::endl;
        for(int b = 0; b < BANK_COUNT; b++){
            splitInputStreams[b].write(input);
        }
    }
}

void total_voter(hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &inferenceOuputStream, const int size)
{
    static const ap_ufixed<24,0> reciprocal = 1.0 / BANK_COUNT;
    for (int i = 0; i < size; i++) {
        ap_ufixed<24, 16> classSums[CLASS_COUNT] = {0};
        #pragma HLS ARRAY_PARTITION variable=classSums dim=1 type=complete
        for(int b = 0; b < BANK_COUNT; b++){
            ClassDistribution result = splitInferenceOutputStreams[b].read();
            for(int c = 0; c < CLASS_COUNT; c++){
                #pragma HLS UNROLL
                classSums[c] += result.distribution[c];
            }
        }

        ClassDistribution avg;
        #pragma HLS ARRAY_PARTITION variable=avg.distribution dim=1 type=complete
        for(int c = 0; c < CLASS_COUNT; c++){
            #pragma HLS UNROLL
            avg.distribution[c] = classSums[c] * reciprocal;
        }
        
        //TODO change to reduction tree
        Result finalResult{0, avg.distribution[0]};
        // ap_ufixed<9,1> max_confidence = avg.distribution[0];
        // int resultClass = 0;
        for(int c = 1; c < CLASS_COUNT; c++){
            #pragma HLS PIPELINE II=1
            if(avg.distribution[c] > finalResult.confidence){
                finalResult.confidence = avg.distribution[c];
                finalResult.resultClass = c;
            }
        }
        inferenceOuputStream.write(finalResult);

    }
}

Feedback::Feedback(const PageProperties &p, const bool &extraPage) : input(p.input), treeID(p.treeID), pageIdx(p.nextPageIdx), extraPage(extraPage), needNewPage(p.needNewPage), freePageIdx(p.freePageIdx)
{
    for(int c = 0; c < CLASS_COUNT; c++){
        parentG[c] = p.parentG[c];
    }
}