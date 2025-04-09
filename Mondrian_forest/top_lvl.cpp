#include "top_lvl.hpp"
#include "processing_unit.hpp"
#include "rng.hpp"
#include <hls_np_channel.h>

void inputSplitter(hls::stream<input_t> &inputStream, hls::stream<input_t> splitInputStreams[BANK_COUNT], const int totalSize);
void total_voter(hls::stream<ClassDistribution> &splitInferenceOutputStreams, hls::stream<Result> &resultOutputStream, const int size);

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &resultOutputStream,
    const InputSizes &sizes,
    PageBank hbmMemory[BANK_COUNT]
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_none port=sizes
    //#pragma HLS INTERFACE m_axi port=hbmMemory depth=BANK_COUNT bundle=hbm
    #pragma HLS ARRAY_PARTITION variable=hbmMemory dim=1 type=complete

    //hls::split::load_balance<unit_interval, 2, 10> rngStream("rngStream");
    hls::stream<input_t> splitInputStreams[BANK_COUNT];
    
    hls::stream<unit_interval, 20> rngStream[BANK_COUNT][TRAIN_TRAVERSAL_BLOCKS];
    hls::merge::round_robin<ClassDistribution, BANK_COUNT> splitInferenceOutputStreams;
    //hls::split::load_balance<unit_interval, BANK_COUNT> rngStream;

    rng_generator(rngStream);
    inputSplitter(inputStream, splitInputStreams, sizes.total);
    
    //hls::task rngTask(rng_generator, rngStream.in);
    for(int b = 0; b < BANK_COUNT; b++){
        #pragma HLS UNROLL
        processing_unit(splitInputStreams[b], rngStream[b], hbmMemory[b], sizes, splitInferenceOutputStreams.in[b]);
    }

    total_voter(splitInferenceOutputStreams.out, resultOutputStream, sizes.inference);

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

void total_voter(hls::stream<ClassDistribution> &splitInferenceOutputStreams, hls::stream<Result> &resultOutputStream, const int size)
{
    static const ap_ufixed<24,0> reciprocal = 1.0 / TREES_PER_BANK;
    ap_ufixed<24, 16> classSums[CLASS_COUNT] = {0};
    for(int b = 0; b < BANK_COUNT;b++){
        ClassDistribution distribution = splitInferenceOutputStreams.read();
        for(int c = 0; c < CLASS_COUNT; c++){
            #pragma HLS PIPELINE II=1
            classSums[c] += distribution.dis[c];
        }
    }
    ClassDistribution avg;
    
    for(int c = 0; c < CLASS_COUNT; c++){
        avg.dis[c] = classSums[c] * reciprocal;
    }
    
    //TODO change to reduction tree
    Result finalResult{0, avg.dis[0]};
    // ap_ufixed<9,1> max_confidence = avg.distribution[0];
    // int resultClass = 0;
    for(int c = 1; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        if(avg.dis[c] > finalResult.confidence){
            finalResult.confidence = avg.dis[c];
            finalResult.resultClass = c;
        }
    }
    resultOutputStream.write(finalResult);

}

Feedback::Feedback(const PageProperties &p, const bool &extraPage) : input(p.input), treeID(p.treeID), pageIdx(p.nextPageIdx), extraPage(extraPage), needNewPage(p.needNewPage), freePageIdx(p.freePageIdx)
{
    for(int c = 0; c < CLASS_COUNT; c++){
        parentG[c] = p.parentG[c];
    }
}