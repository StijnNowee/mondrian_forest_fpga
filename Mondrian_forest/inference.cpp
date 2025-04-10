#include "inference.hpp"
#include "processing_unit.hpp"
#include "converters.hpp"
#include <ap_fixed.h>
#include <hls_math.h>
#include <hls_np_channel.h>



void infer_tree(hls::stream_of_blocks<IPage> &pageIn, hls::stream<IFeedback> &output);
void multiplexer(hls::stream<IFeedback> &input, hls::stream<IFeedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput);

void inference(hls::stream<IFetchRequest> &fetchRequestStream, hls::stream<IFeedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput, const PageBank &pageBank)
{   
    #pragma HLS DATAFLOW disable_start_propagation
    hls::stream_of_blocks<IPage> traversalStreams[INF_TRAVERSAL_BLOCKS];
    hls::merge::load_balance<IFeedback, INF_TRAVERSAL_BLOCKS> inferOut;
    fetcher<INF_TRAVERSAL_BLOCKS, IFetchRequest, IPageProperties>(fetchRequestStream, traversalStreams, pageBank);
    for(int i = 0; i < INF_TRAVERSAL_BLOCKS; i++){
       #pragma HLS UNROLL
        infer_tree(traversalStreams[i], inferOut.in[i]);
    }
    multiplexer(inferOut.out, feedbackStream, voterOutput);
}

void infer_tree(hls::stream_of_blocks<IPage> &pageIn, hls::stream<IFeedback> &output)
{
    if(!pageIn.empty()){
        hls::read_lock<IPage> page(pageIn);
        
        ap_ufixed<16,1> p_notseperated = 1;
        bool endReached = false;
        int nextNodeIdx = 0;
        IFeedback feedback(rawToProperties<IPageProperties>(page[MAX_NODES_PER_PAGE]));

        while(!endReached){
            const Node_hbm node = rawToNode(page[nextNodeIdx]);
            const splitT_t tdiff = node.splittime - node.parentSplitTime;
            rate_t rate = 0;
            for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                auto tmpUpper = (feedback.input.feature[d] > node.upperBound[d]) ? (feedback.input.feature[d] - node.upperBound[d]) : ap_fixed<9,1>(0);
                auto tmpLower = (node.lowerBound[d] > feedback.input.feature[d]) ? (node.lowerBound[d] - feedback.input.feature[d]) : ap_fixed<9,1>(0);
                rate += tmpUpper + tmpLower;
            }
            ap_ufixed<16,1> probInverted = hls::exp(-tdiff*rate); 
            ap_ufixed<16,1> prob = 1 - probInverted;
            if(prob > 0){
                for(int c = 0; c < CLASS_COUNT; c++){
                    feedback.s.dis[c] += p_notseperated*prob*node.weight[c];
                }
            }

            if(node.leaf()){
                for(int c = 0; c < CLASS_COUNT; c++){
                    feedback.s.dis[c] += p_notseperated*probInverted*node.weight[c];
                }
                feedback.isOutput = true;    
                endReached = true;     
            }else{
                p_notseperated *= probInverted;
                
                ChildNode child;
                if(feedback.input.feature[node.feature] <= node.threshold){
                    child = node.leftChild;
                }else{
                    child = node.rightChild;
                }
                if (!child.isPage()) {
                    nextNodeIdx = child.id();
                } else {
                    feedback.pageIdx = child.id();
                    feedback.needNewPage = true;
                    endReached = true;
                }

            }
        }
        output.write(feedback);
    }
}

void multiplexer(hls::stream<IFeedback> &input, hls::stream<IFeedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput)
{
    if(!input.empty()){
        auto inputV = input.read();
        feedbackStream.write(inputV);
        if(inputV.isOutput){
            voterOutput.write(inputV.s);
        }
    }
}
