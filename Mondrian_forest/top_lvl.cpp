#include "top_lvl.hpp"
#include "processing_unit.hpp"
#include "rng.hpp"

void inputSplitter(hls::stream<input_t> &inputStream, hls::stream<input_t> splitInputStreams[BANK_COUNT], const int totalSize, hls::stream<bool> &doneStream);
void voter(hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &resultOutputStream, const int size);
void process_inference_output(hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &resultOutputStream, const ap_ufixed<24,1> &reciprocal);

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &resultOutputStream,
    const InputSizes &sizes,
    PageBank trainHBM[BANK_COUNT],
    PageBank inferenceHBM[BANK_COUNT]
)  {
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE ap_none port=sizes
    
    #pragma HLS ARRAY_PARTITION variable=trainHBM dim=1 type=complete
    #pragma HLS ARRAY_PARTITION variable=inferenceHBM dim=1 type=complete

    hls::stream<input_t> splitInputStreams[BANK_COUNT];
    
    hls::stream<unit_interval, 20> rngStream[BANK_COUNT][TRAIN_TRAVERSAL_BLOCKS];
    hls::stream<ClassDistribution, TREES_PER_BANK> splitInferenceOutputStreams[BANK_COUNT];
    hls::stream<bool> doneStream("doneStream");

    static int maxPageNr[BANK_COUNT][TREES_PER_BANK] = {0};
    #pragma HLS ARRAY_PARTITION variable=maxPageNr dim=1 type=complete
    
    inputSplitter(inputStream, splitInputStreams, sizes.total, doneStream);
    rng_generator(rngStream, doneStream);
    
    for(int b = 0; b < BANK_COUNT; b++){
        #pragma HLS UNROLL
        processing_unit(splitInputStreams[b], rngStream[b], trainHBM[b], inferenceHBM[b], sizes, splitInferenceOutputStreams[b], maxPageNr[b]);
    }

    voter(splitInferenceOutputStreams, resultOutputStream, sizes.inference);

}

void inputSplitter(hls::stream<input_t> &inputStream, hls::stream<input_t> splitInputStreams[BANK_COUNT], const int totalSize, hls::stream<bool> &doneStream)
{
    for(int i = 0; i < totalSize; i++){
        auto input = inputStream.read();
        for(int b = 0; b < BANK_COUNT; b++){
            splitInputStreams[b].write(input);
        }
    }
    doneStream.write(true);
}

void voter(hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &resultOutputStream, const int size)
{
    static const ap_ufixed<24,1> reciprocal = 1.0 / (TREES_PER_BANK*BANK_COUNT);
    for(int i = 0; i < size*BANK_COUNT; i++){
        process_inference_output(splitInferenceOutputStreams, resultOutputStream, reciprocal);
    }
    
}

void process_inference_output(hls::stream<ClassDistribution> splitInferenceOutputStreams[BANK_COUNT], hls::stream<Result> &resultOutputStream, const ap_ufixed<24,1> &reciprocal)
{
    ap_ufixed<16, 8> classSums[CLASS_COUNT] = {0};
    for(int t = 0; t < TREES_PER_BANK; t++){
        for(int b = 0; b < BANK_COUNT;b++){
            ClassDistribution distribution = splitInferenceOutputStreams[b].read();
            for(int c = 0; c < CLASS_COUNT; c++){
                #pragma HLS PIPELINE II=1
                classSums[c] += distribution.dis[c];
            }
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
}

IFeedback::IFeedback(const IPageProperties &p) : Feedback(p, false), isOutput(p.isOutput){
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        s.dis[c] = p.s.dis[c];
    }
}