#include "train.hpp"
#include <ap_fixed.h>
#include <hls_math.h>
#include <cwchar>
#include "converters.hpp"
#include <iostream>

void calculate_e_values(const Node_hbm &node, const input_vector &input, unit_interval e_l[FEATURE_COUNT_TOTAL], unit_interval e_u[FEATURE_COUNT_TOTAL], unit_interval e[FEATURE_COUNT_TOTAL], rate_t e_cum[FEATURE_COUNT_TOTAL], rate_t &rate);
int determine_split_dimension(const rate_t &rngValue, rate_t e_cum[FEATURE_COUNT_TOTAL]);
bool traverse(Node_hbm &node, PageProperties &p, unit_interval e_l[FEATURE_COUNT_TOTAL], unit_interval e_u[FEATURE_COUNT_TOTAL], int &nextNodeIdx, const posterior_t &parentG);
void update_parent_posterior(posterior_t &parent, const posterior_t &newParent);
void update_internal_posterior_predictive_distribution(Node_hbm &node, const posterior_t &parentG);
void update_leaf_posterior_predictive_distribution(Node_hbm &node, const posterior_t &parentG);

void tree_traversal(hls::stream_of_blocks<IPage> &pageInS, hls::stream<unit_interval> &rngStream, hls::stream_of_blocks<IPage> &pageOutS)
{
    if(!pageInS.empty()){
        unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL], e[FEATURE_COUNT_TOTAL];
        rate_t e_cum[FEATURE_COUNT_TOTAL];
        hls::read_lock<IPage> pageIn(pageInS);
        hls::write_lock<IPage> pageOut(pageOutS);

        for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
            pageOut[n] = pageIn[n];
        }
        PageProperties p = rawToProperties(pageIn[MAX_NODES_PER_PAGE]);

        int nextNodeIdx = 0;
        int parentIdx = 0;

        //Traverse down the page
        bool endReached = false;
        tree_loop: while(!endReached){
            #pragma HLS LOOP_TRIPCOUNT max=MAX_DEPTH min=1
            Node_hbm node(rawToNode(pageOut[nextNodeIdx]));
            rate_t rate = 0;
            calculate_e_values(node, p.input, e_l, e_u, e, e_cum, rate);
            splitT_t E;
            if(rate != 0){
                ap_fixed<16, 4> randomValue = 1 - rngStream.read();
                ap_ufixed<16, 4, AP_TRN, AP_SAT> tmp = -hls::log(randomValue);
                ap_ufixed<16, 12,AP_TRN, AP_SAT> tmp2 = tmp/rate;
                E = tmp2;
            }

            if(rate != 0 && node.parentSplitTime + E < node.splittime){
                //Prepare for split
                rate_t rng_val = rngStream.read() * rate;
                p.setSplitProperties(node.idx(), determine_split_dimension(rng_val, e_cum), parentIdx, (node.parentSplitTime + E), rngStream.read());
                endReached = true;
            }else{
                //Traverse
                parentIdx = node.idx();
                endReached = traverse(node, p, e_l, e_u, nextNodeIdx, p.parentG);
                update_parent_posterior(p.parentG, node.posteriorP);
                pageOut[node.idx()] = nodeToRaw(node);
            }
        }
        pageOut[MAX_NODES_PER_PAGE] = propertiesToRaw(p);
    }
}


void calculate_e_values(const Node_hbm &node, const input_vector &input, unit_interval e_l[FEATURE_COUNT_TOTAL], unit_interval e_u[FEATURE_COUNT_TOTAL], unit_interval e[FEATURE_COUNT_TOTAL], rate_t e_cum[FEATURE_COUNT_TOTAL], rate_t &rate)
{
    #pragma HLS inline
    calculate_e_values: for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        #pragma HLS PIPELINE II=1
        e_l[d] = (node.lowerBound[d] > input.feature[d]) ? unit_interval(node.lowerBound[d] - input.feature[d]) : unit_interval(0);
        e_u[d] = (input.feature[d] > node.upperBound[d]) ? unit_interval(input.feature[d] - node.upperBound[d]) : unit_interval(0);
        e[d] = e_l[d] + e_u[d];
        rate += e_l[d] + e_u[d];
        e_cum[d] = rate;
    }
}

int determine_split_dimension(const rate_t &rngValue, rate_t e_cum[FEATURE_COUNT_TOTAL])
{
    #pragma HLS inline
    int splitDimension = UNDEFINED_DIMENSION;
    split_dimension: for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        if(rngValue <= e_cum[d] && splitDimension == UNDEFINED_DIMENSION){
            splitDimension = d;
        }
    }
    return splitDimension;
}

bool traverse(Node_hbm &node, PageProperties &p, unit_interval e_l[FEATURE_COUNT_TOTAL], unit_interval e_u[FEATURE_COUNT_TOTAL], int &nextNodeIdx, const posterior_t &parentG)
{
    // #pragma HLS inline
    bool end_reached = false;
    update_bounds: for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        #pragma HLS PIPELINE II=1
        node.lowerBound[d] = (e_l[d] !=0) ? p.input.feature[d] : node.lowerBound[d];
        node.upperBound[d] = (e_u[d] !=0) ? p.input.feature[d] : node.upperBound[d];
    }

    if(node.leaf()){
        if(node.counts[p.input.label] < 255){
            node.counts[p.input.label]++;
        }
        update_leaf_posterior_predictive_distribution(node, parentG);
        end_reached = true;
    }else{
        //Traverse
        
        ChildNode child;
        if(p.input.feature[node.feature] <= node.threshold){
            node.setTab(LEFT, p.input.label);
            child = node.leftChild;
        }else{
            node.setTab(RIGHT, p.input.label);
            child = node.rightChild;
        }
        if (!child.isPage()) {
            nextNodeIdx = child.id();
        } else {
            p.nextPageIdx = child.id();
            p.needNewPage = true;
            end_reached = true;
        }
        update_internal_posterior_predictive_distribution(node, parentG);
    }
    return end_reached;
}

void update_parent_posterior(posterior_t &parent, const posterior_t &newParent)
{
    for(int c = 0; c < CLASS_COUNT; c++){
        parent[c] = newParent[c];
    }
}

void update_internal_posterior_predictive_distribution(Node_hbm &node, const posterior_t &parentG)
{
    ap_ufixed<32, 0> discount = hls::exp(-GAMMA*(node.splittime - node.parentSplitTime));
    int totalCount = 0;
    int countPerClass[CLASS_COUNT];
    for(int c = 0; c < CLASS_COUNT; c++){
        countPerClass[c] = node.getTab(LEFT, c) + node.getTab(RIGHT, c);
        totalCount += countPerClass[c];
    }
    ap_ufixed<32, 0> oneoverCount = 1/totalCount;
    for(int c = 0; c < CLASS_COUNT; c++){
        node.posteriorP[c] = oneoverCount*(countPerClass[c] - discount*countPerClass[c] + discount*totalCount*parentG[c]);
    }
}

void update_leaf_posterior_predictive_distribution(Node_hbm &node, const posterior_t &parentG)
{
    ap_ufixed<32, 0> discount = hls::exp(-GAMMA*(node.splittime - node.parentSplitTime));

    int totalCount = 0;
    int totalTabs = 0;
    for(int c = 0; c < CLASS_COUNT; c++){
        totalCount += node.counts[c];
        totalTabs += node.counts[c] > 0; 
    }
    ap_ufixed<32, 0> oneoverCount = 1/totalCount;
    for(int c = 0; c < CLASS_COUNT; c++){
        node.posteriorP[c] = oneoverCount*(node.counts[c] - discount*(node.counts[c] > 0) + discount*totalTabs*parentG[c]);
    }
}