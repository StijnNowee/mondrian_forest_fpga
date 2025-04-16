#include "inference.hpp"
#include "processing_unit.hpp"
#include "converters.hpp"
#include <hls_math.h>



void infer_tree(hls::stream_of_blocks<IPage> &pageIn, hls::stream<IFeedback> &output);
void inferenceFetcher(hls::stream<IFetchRequest> &fetchRequestStream,  hls::stream_of_blocks<IPage> &traversalStream , const Page *pageBank);

void inference(hls::stream<IFetchRequest> &fetchRequestStream, hls::stream<IFeedback> &feedbackStream, const Page *pageBank, const int &blockIdx)
{   
    #pragma HLS DATAFLOW disable_start_propagation
    hls::stream_of_blocks<IPage> traversalStream;
    inferenceFetcher(fetchRequestStream, traversalStream, pageBank);
    infer_tree(traversalStream, feedbackStream);

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
            //#pragma HLS PIPELINE II=6
            const Node_hbm node = rawToNode(page[nextNodeIdx]);
            const splitT_t tdiff = node.splittime - node.parentSplitTime;
            rate_t rate = 0;
            for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                #pragma HLS PIPELINE II=1
                auto tmpUpper = (feedback.input.feature[d] > node.upperBound[d]) ? (feedback.input.feature[d] - node.upperBound[d]) : ap_fixed<9,1>(0);
                auto tmpLower = (node.lowerBound[d] > feedback.input.feature[d]) ? (node.lowerBound[d] - feedback.input.feature[d]) : ap_fixed<9,1>(0);
                rate += tmpUpper + tmpLower;
            }
            ap_ufixed<16,1> probInverted = hls::exp(-tdiff*rate); 
            ap_ufixed<16,1> prob = 1 - probInverted;
            if(prob > 0){
                for(int c = 0; c < CLASS_COUNT; c++){
                    #pragma HLS PIPELINE II=1
                    feedback.s.dis[c] += p_notseperated*prob*node.weight[c];
                }
            }

            if(node.leaf()){
                for(int c = 0; c < CLASS_COUNT; c++){
                    #pragma HLS PIPELINE II=1
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

void inferenceFetcher(hls::stream<IFetchRequest> &fetchRequestStream,  hls::stream_of_blocks<IPage> &traversalStream , const Page *pageBank)
{
    if(!fetchRequestStream.empty()){
        hls::write_lock<IPage> pageOut(traversalStream);
        IFetchRequest request = fetchRequestStream.read();
        const int globalPageIdx = request.treeID * MAX_PAGES_PER_TREE + request.pageIdx;
        for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            #pragma HLS PIPELINE II=1
            pageOut[i] = pageBank[globalPageIdx][i];
            IPageProperties p(request);
            pageOut[MAX_NODES_PER_PAGE] = propertiesToRaw<IPageProperties>(p);
        }
    }
}