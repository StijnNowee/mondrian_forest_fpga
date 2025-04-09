#include "inference.hpp"
#include "processing_unit.hpp"
#include "converters.hpp"
#include <hls_math.h>



void infer_tree(hls::stream_of_blocks<IPage> &pageIn, hls::stream<Feedback> &output);
void voter(hls::stream<Feedback> input[INF_TRAVERSAL_BLOCKS], hls::stream<Feedback> &feedbackStream, hls::stream<ClassDistribution> &voterOutput);
ap_ufixed<32,1> calculate_expected_discount(const splitT_t &tdiff, const rate_t &rate);
void branch_off(Node_hbm &node, const splitT_t &tdiff,const rate_t &rate, const posterior_t &parentG, posterior_t &s, const ap_ufixed<16,1> &p_notseperated, const ap_ufixed<16,1> &prob);

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
    posterior_t s = {0};
    ap_ufixed<16,1> p_notseperated = 1;
    bool endReached = true;
    int nextNodeIdx = 0;
    while(!endReached){
        Node_hbm node = rawToNode(page[nextNodeIdx]);
        splitT_t tdiff = node.splittime - node.parentSplitTime;
        rate_t rate = 0;
        for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
            //If answer is negative, it is set at 0, aka MAX(x, 0)
            rate += (p.input.feature[d] - node.upperBound[d]) + (node.lowerBound[d] - p.input.feature[d]);
        }
        ap_ufixed<16,1> probInverted = hls::exp(-tdiff*rate); 
        ap_ufixed<16,1> prob = 1 - probInverted;
        if(prob > 0){
            branch_off(node, tdiff, rate, p.parentG, s, p_notseperated, prob);
        }

        if(node.leaf()){
            Feedback feedback;
            for(int c = 0; c < CLASS_COUNT; c++){
                feedback.parentG[c] = s[c] + p_notseperated*probInverted*node.posteriorP[c];
            }
            feedback.isOutput = true;
            output.write(feedback);            
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
                p.nextPageIdx = child.id();
                p.needNewPage = true;
                endReached = true;
            }
            for(int c = 0; c < CLASS_COUNT; c++){
                #pragma HLS PIPELINE II=1
                p.parentG[c] = node.posteriorP[c];
    }
        }

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
        s[c] += p_notseperated * prob * G;
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
