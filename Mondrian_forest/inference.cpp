#include "inference.hpp"
#include "processing_unit.hpp"
#include "converters.hpp"



void infer_tree(hls::stream_of_blocks<IPage> &pageIn, hls::stream<Feedback> &output);
void voter(hls::stream<Feedback> input[INF_TRAVERSAL_BLOCKS], hls::stream<Feedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput);

void inference(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<Feedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput, const PageBank &pageBank)
{   
    #pragma HLS DATAFLOW disable_start_propagation
    hls::stream_of_blocks<IPage> traversalStreams[INF_TRAVERSAL_BLOCKS];
    hls::stream<Feedback> inferOut[INF_TRAVERSAL_BLOCKS];
    fetcher<INF_TRAVERSAL_BLOCKS>(fetchRequestStream, traversalStreams, pageBank);
    for(int i = 0; i < INF_TRAVERSAL_BLOCKS; i++){
       #pragma HLS UNROLL
        infer_tree(traversalStreams[i], inferOut[i]);
    }
    voter(inferOut, feedbackStream, voterOutput);
}

void infer_tree(hls::stream_of_blocks<IPage> &pageIn, hls::stream<Feedback> &output)
{
    hls::read_lock<IPage> page(pageIn);
    PageProperties p = rawToProperties(page[MAX_NODES_PER_PAGE]);
}

void voter(hls::stream<ClassDistribution> traversalOutputStream[TREES_PER_BANK],  hls::stream<ClassDistribution> &resultOutputStream, const int size)
{

    static const ap_ufixed<24,0> reciprocal = 1.0 / TREES_PER_BANK;
    for (int i = 0; i < size; i++) {
        ap_ufixed<24, 16> classSums[CLASS_COUNT] = {0};
        for(int t = 0; t < TREES_PER_BANK; t++){
            #pragma HLS UNROLL off
            ClassDistribution distribution = traversalOutputStream[t].read();
            for(int c = 0; c < CLASS_COUNT; c++){
                #pragma HLS PIPELINE II=1
                classSums[c] += distribution.distribution[c];
            }
        }
        ClassDistribution avg;
        #pragma HLS ARRAY_PARTITION variable=avg.distribution dim=1 type=complete
        
        for(int c = 0; c < CLASS_COUNT; c++){
            #pragma HLS UNROLL
            avg.distribution[c] = classSums[c] * reciprocal;
        }
        resultOutputStream.write(avg);
    }
}
