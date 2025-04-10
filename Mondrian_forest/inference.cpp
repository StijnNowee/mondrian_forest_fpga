#include "inference.hpp"
#include "processing_unit.hpp"
#include "converters.hpp"
#include <hls_math.h>
#include <hls_np_channel.h>



void infer_tree(hls::stream_of_blocks<IPage> &pageIn, hls::stream<IFeedback> &output);
void multiplexer(hls::stream<IFeedback> &input, hls::stream<IFeedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput);
ap_ufixed<32,1> calculate_expected_discount(const splitT_t &tdiff, const rate_t &rate);
void branch_off(Node_hbm &node, const splitT_t &tdiff,const rate_t &rate, const posterior_t &parentG, posterior_t &s, const ap_ufixed<16,1> &p_notseperated, const ap_ufixed<16,1> &prob);

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
            Node_hbm node = rawToNode(page[nextNodeIdx]);
            splitT_t tdiff = node.splittime - node.parentSplitTime;
            rate_t rate = 0;
            for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                auto tmpUpper = (feedback.input.feature[d] > node.upperBound[d]) ? (feedback.input.feature[d] - node.upperBound[d]) : ap_fixed<9,1>(0);
                auto tmpLower = (node.lowerBound[d] > feedback.input.feature[d]) ? (node.lowerBound[d] - feedback.input.feature[d]) : ap_fixed<9,1>(0);
                rate += tmpUpper + tmpLower;
            }
            ap_ufixed<16,1> probInverted = hls::exp(-tdiff*rate); 
            ap_ufixed<16,1> prob = 1 - probInverted;
            if(prob > 0){
                branch_off(node, tdiff, rate, feedback.parentG, feedback.s.dis, p_notseperated, prob);
            }

            if(node.leaf()){
                for(int c = 0; c < CLASS_COUNT; c++){
                    feedback.s.dis[c] += p_notseperated*probInverted*node.posteriorP[c];
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
                for(int c = 0; c < CLASS_COUNT; c++){
                    #pragma HLS PIPELINE II=1
                    feedback.parentG[c] = node.posteriorP[c];
                }
            }
        }
        output.write(feedback);
    }
}

void branch_off(Node_hbm &node, const splitT_t &tdiff,const rate_t &rate, const posterior_t &parentG, posterior_t &s, const ap_ufixed<16,1> &p_notseperated, const ap_ufixed<16,1> &prob)
{
    auto discount = calculate_expected_discount(tdiff, rate);
    int totalCount = 0;
    ap_uint<CLASS_BITS> totalTabs = 0;
    ap_byte_t tmpCount[CLASS_COUNT];
    ap_uint<1> tmpTabs[CLASS_COUNT];

    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=2
        tmpCount[c] = (node.leaf()) ? node.counts[c] : ap_byte_t(node.getTab(LEFT, c) + node.getTab(RIGHT, c));
        totalCount += tmpCount[c];
        tmpTabs[c] = tmpCount[c] > 0;
        totalTabs += tmpTabs[c];
    }
    ap_ufixed<16,1> oneoverCount = 1.0/totalCount;
    for(int c = 0; c < CLASS_COUNT; c++){
        bool tab = (node.counts[c] > 0);
        ap_ufixed<32,1> G = oneoverCount * (tmpCount[c] - discount*tmpTabs[c] + discount * totalTabs * parentG);
        ap_ufixed<8,0,AP_TRN, AP_SAT> tmp =  p_notseperated * prob * G;
        s[c] += tmp;
    }
}

ap_ufixed<32,1> calculate_expected_discount(const splitT_t &tdiff, const rate_t &rate)
{
    // auto tmp = -(GAMMA + rate) * tdiff;
    // ap_ufixed<32,0> term1 = -hls::expm1(tmp);
    // auto tmp2 = -rate * tdiff;
    // ap_ufixed<32,0> term2 = -hls::expm1(tmp2);

    // ap_ufixed<16,1> factor1 = rate / (GAMMA + rate);
    
    // ap_ufixed<32,1> factor2 = (term2 !=0 ) ? ap_ufixed<32,1>(term1 / term2) : ap_ufixed<32,1>(0);
    float tmp = -(GAMMA + rate) * tdiff;
    float term1 = -hls::expm1(tmp);
    float tmp2 = -rate * tdiff;
    float term2 = -hls::expm1(tmp2);

    float factor1 = rate / (GAMMA + rate);
    
    float factor2 = (term2 !=0 ) ? ap_ufixed<32,1>(term1 / term2) : ap_ufixed<32,1>(0);

    return factor1*factor2;
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
