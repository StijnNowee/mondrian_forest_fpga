#include "inference.hpp"
#include "processing_unit.hpp"
#include "converters.hpp"
#include <hls_math.h>
#include <hls_np_channel.h>



void infer_tree(hls::stream_of_blocks<IPage> &pageIn, hls::stream<Feedback> &output);
void voter(hls::stream<Feedback> &input, hls::stream<Feedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput);
ap_ufixed<32,1> calculate_expected_discount(const splitT_t &tdiff, const rate_t &rate);
void branch_off(Node_hbm &node, const splitT_t &tdiff,const rate_t &rate, const posterior_t &parentG, posterior_t &s, const ap_ufixed<16,1> &p_notseperated, const ap_ufixed<16,1> &prob);

void inference(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<Feedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput, const PageBank &pageBank)
{   
    #pragma HLS DATAFLOW disable_start_propagation
    hls::stream_of_blocks<IPage> traversalStreams[INF_TRAVERSAL_BLOCKS];
    hls::merge::load_balance<Feedback, INF_TRAVERSAL_BLOCKS> inferOut;
    fetcher<INF_TRAVERSAL_BLOCKS>(fetchRequestStream, traversalStreams, pageBank);
    for(int i = 0; i < INF_TRAVERSAL_BLOCKS; i++){
       #pragma HLS UNROLL
        infer_tree(traversalStreams[i], inferOut.in[i]);
    }
    voter(inferOut.out, feedbackStream, voterOutput);
}

void infer_tree(hls::stream_of_blocks<IPage> &pageIn, hls::stream<Feedback> &output)
{
    if(!pageIn.empty()){
        hls::read_lock<IPage> page(pageIn);
        const PageProperties p = rawToProperties(page[MAX_NODES_PER_PAGE]);
        posterior_t s = {0};
        ap_ufixed<16,1> p_notseperated = 1;
        bool endReached = false;
        int nextNodeIdx = 0;
        Feedback feedback(p, false);

        while(!endReached){
            Node_hbm node = rawToNode(page[nextNodeIdx]);
            splitT_t tdiff = node.splittime - node.parentSplitTime;
            rate_t rate = 0;
            for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                auto tmpUpper = (p.input.feature[d] > node.upperBound[d]) ? (p.input.feature[d] - node.upperBound[d]) : ap_fixed<9,1>(0);
                auto tmpLower = (node.lowerBound[d] > p.input.feature[d]) ? (node.lowerBound[d] - p.input.feature[d]) : ap_fixed<9,1>(0);
                rate += tmpUpper + tmpLower;
            }
            ap_ufixed<16,1> probInverted = hls::exp(-tdiff*rate); 
            ap_ufixed<16,1> prob = 1 - probInverted;
            if(prob > 0){
                branch_off(node, tdiff, rate, p.parentG, s, p_notseperated, prob);
            }

            if(node.leaf()){
                for(int c = 0; c < CLASS_COUNT; c++){
                    feedback.parentG[c] = s[c] + p_notseperated*probInverted*node.posteriorP[c];
                }
                feedback.isOutput = true;    
                endReached = true;     
            }else{
                p_notseperated *= probInverted;
                ChildNode child;
                if(p.input.feature[node.feature] <= node.threshold){
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
    ap_ufixed<16,1> oneoverCount = 1/totalCount;
    for(int c = 0; c < CLASS_COUNT; c++){
        bool tab = (node.counts[c] > 0);
        ap_ufixed<32,1> G = oneoverCount * (tmpCount[c] - discount*tmpTabs[c] + discount * totalTabs * parentG);
        ap_ufixed<8,0,AP_TRN, AP_SAT> tmp =  p_notseperated * prob * G;
        s[c] += tmp;
    }
}

ap_ufixed<32,1> calculate_expected_discount(const splitT_t &tdiff, const rate_t &rate)
{
    auto tmp = -(GAMMA + rate) * tdiff;
    ap_ufixed<32,0> term1 = 1 - hls::exp(tmp);
    auto tmp2 = -rate * tdiff;
    ap_ufixed<32,0> term2 = 1 - hls::exp(tmp2);

    ap_ufixed<16,1> factor1 = rate / (GAMMA + rate);
    ap_ufixed<32,1> factor2 = term1 / term2;

    return factor1*factor2;
}

void voter(hls::stream<Feedback> &input, hls::stream<Feedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput)
{
    static const ap_ufixed<24,0> reciprocal = 1.0 / TREES_PER_BANK;
    ap_ufixed<24, 16> classSums[CLASS_COUNT] = {0};
    for(int t = 0; t < TREES_PER_BANK;){
        Feedback feedback = input.read();
        if(feedback.isOutput){
            for(int c = 0; c < CLASS_COUNT; c++){
                #pragma HLS PIPELINE II=1
                classSums[c] += feedback.parentG[c];
            }
            t++;
        }else{
            feedbackStream.write(feedback);
        }
    }
    ClassDistribution avg;
    
    for(int c = 0; c < CLASS_COUNT; c++){
        avg.dis[c] = classSums[c] * reciprocal;
    }
    voterOutput.write(avg);
}
