#include "top_lvl.hpp"
#include "processing_unit.hpp"
#include "rng.hpp"

void inputSplitter(hls::stream<input_vector> &inputStream, hls::stream<input_vector> bankInputStream[BANK_COUNT], const int &size);
void voter(hls::stream<ClassSums> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &resultOutputStream, const int size);
// void process_inference_output(hls::stream<ClassDistribution, 4> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &resultOutputStream);

void top_lvl(
    hls::stream<input_vector> inputStream[2],
    hls::stream<Result> &resultOutputStream,
    hls::stream<int> executionCountStream[BANK_COUNT],
    const InputSizes &sizes,
    PageBank trainHBM[BANK_COUNT],
    PageBank inferenceHBM[BANK_COUNT]
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_none port=sizes
    
    #pragma HLS ARRAY_PARTITION variable=trainHBM dim=1 type=complete
    #pragma HLS ARRAY_PARTITION variable=inferenceHBM dim=1 type=complete

    hls::stream<input_vector> bankInputStream[2][BANK_COUNT];
    
    hls::stream<unit_interval, 20> rngStream[BANK_COUNT][TRAIN_TRAVERSAL_BLOCKS];

    hls::stream<ClassSums, 2> splitInferenceOutputStreams[BANK_COUNT];

    static int maxPageNr[BANK_COUNT][TREES_PER_BANK] = {0};
    #pragma HLS ARRAY_PARTITION variable=maxPageNr dim=1 type=complete
    
    for(int i = 0; i < 2; i++){
        #pragma HLS UNROLL
        inputSplitter(inputStream[i], bankInputStream[i], sizes.seperate[i]);
    }
    
    rng_generator(rngStream);
    
    for(int b = 0; b < BANK_COUNT; b++){
        #pragma HLS UNROLL
        processing_unit(bankInputStream[TRAIN][b], bankInputStream[INF][b], rngStream[b], trainHBM[b], inferenceHBM[b], sizes, splitInferenceOutputStreams[b], maxPageNr[b], executionCountStream[b]);
    }

    voter(splitInferenceOutputStreams, resultOutputStream, sizes.seperate[INF]);

}

void inputSplitter(hls::stream<input_vector> &inputStream, hls::stream<input_vector> bankInputStream[BANK_COUNT], const int &size)
{
    for(int i = 0; i < size; i++){
        auto input = inputStream.read();
        for(int b = 0; b < BANK_COUNT; b++){
            #pragma HLS PIPELINE II=1
            bankInputStream[b].write(input);
        }
    }
}

void voter(hls::stream<ClassSums> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &resultOutputStream, const int size)
{
    
    static const ap_ufixed<24,1> reciprocal = 1.0 / (TREES_PER_BANK*BANK_COUNT);
    for(int i = 0; i < size; i++){
        totalSum_t totalClassSum = {0};
            for(int b = 0; b < BANK_COUNT;b++){
                auto localSum = splitInferenceOutputStreams[b].read();
                for(int c = 0; c < CLASS_COUNT; c++){
                    #pragma HLS PIPELINE II=1
                    totalClassSum[c] += localSum.classSums[c];
                }
        }
        ClassDistribution avg;
        
        for(int c = 0; c < CLASS_COUNT; c++){
            avg.dis[c] = totalClassSum[c] * reciprocal;
        }
        
        Result finalResult{0, avg.dis[0]};
        for(int c = 1; c < CLASS_COUNT; c++){
            #pragma HLS PIPELINE II=1
            if(avg.dis[c] > finalResult.confidence){
                finalResult.confidence = avg.dis[c];
                finalResult.resultClass = c;
            }
        }
        resultOutputStream.write(finalResult);
    }
}

Feedback::Feedback(const PageProperties &p, const bool &extraPage) : input(p.input), treeID(p.treeID), pageIdx(p.nextPageIdx), extraPage(extraPage), needNewPage(p.needNewPage), freePageIdx(p.freePageIdx)
{
}

IFeedback::IFeedback(const IPageProperties &p) : Feedback(p, false), isOutput(p.isOutput){
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        s.dis[c] = p.s.dis[c];
    }
}