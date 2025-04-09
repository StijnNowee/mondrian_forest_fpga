#include "train.hpp"
#include <ap_fixed.h>
#include <hls_math.h>
#include <cwchar>
#include "converters.hpp"
#include <iostream>
#include "processing_unit.hpp"

void calculate_e_values(const Node_hbm &node, const input_vector &input, rate_t e_cum[FEATURE_COUNT_TOTAL], rate_t &rate);
int determine_split_dimension(const rate_t &rngValue, rate_t e_cum[FEATURE_COUNT_TOTAL]);
bool traverse(Node_hbm &node, PageProperties &p, int &nextNodeIdx, const posterior_t &parentG);
void update_parent_posterior(posterior_t &parent, const posterior_t &newParent);
bool allLabelsIdentical(const ap_byte_t counts[CLASS_COUNT], int &label);
bool process_active_node(Node_hbm &node, PageProperties &p, hls::stream<unit_interval> &rngStream, int &parentIdx, int &nextNodeIdx, IPage &page);
void update_extend(Node_hbm &node, PageProperties &p);
void process_pauzed_node(Node_hbm &node, PageProperties &p);
void extend_mondrian_block(IPage &page, PageProperties &p, hls::stream<unit_interval> &rngStream);
void sample(splitT_t &E, const rate_t &rate, hls::stream<unit_interval> &rngStream);

void tree_traversal(hls::stream_of_blocks<IPage> &pageInS, hls::stream<unit_interval> &rngStream, hls::stream_of_blocks<IPage> &pageOutS)
{
    if(!pageInS.empty()){
        hls::read_lock<IPage> pageIn(pageInS);
        hls::write_lock<IPage> pageOut(pageOutS);

        for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
            pageOut[n] = pageIn[n];
        }
        PageProperties p = rawToProperties(pageIn[MAX_NODES_PER_PAGE]);

        extend_mondrian_block(pageOut, p, rngStream);
        
        pageOut[MAX_NODES_PER_PAGE] = propertiesToRaw(p);
    }
}

void extend_mondrian_block(IPage &page, PageProperties &p, hls::stream<unit_interval> &rngStream)
{
    int nextNodeIdx = 0;
    int parentIdx = 0;

    //Traverse down the page
    bool endReached = false;
    tree_loop: while(!endReached){
        #pragma HLS LOOP_TRIPCOUNT max=MAX_DEPTH min=1
        Node_hbm node(rawToNode(page[nextNodeIdx]));
        int label;
        if(allLabelsIdentical(node.counts, label) && label == p.input.label){
            process_pauzed_node(node, p);
            endReached = true;
        }else{
            endReached = process_active_node(node, p, rngStream, parentIdx, nextNodeIdx, page);
        }
        
    }
}

void calculate_e_values(const Node_hbm &node, const input_vector &input, rate_t e_cum[FEATURE_COUNT_TOTAL], rate_t &rate)
{
    #pragma HLS inline
    unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
    calculate_e_values: for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        #pragma HLS PIPELINE II=1
        e_l[d] = (node.lowerBound[d] > input.feature[d]) ? unit_interval(node.lowerBound[d] - input.feature[d]) : unit_interval(0);
        e_u[d] = (input.feature[d] > node.upperBound[d]) ? unit_interval(input.feature[d] - node.upperBound[d]) : unit_interval(0);
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

void update_extend(Node_hbm &node, PageProperties &p)
{
    update_extend: for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        #pragma HLS PIPELINE II=1
        node.upperBound[d] = hls::max(node.upperBound[d], p.input.feature[d]);
        node.lowerBound[d] = hls::min(node.lowerBound[d], p.input.feature[d]);
    }
}

bool traverse(Node_hbm &node, PageProperties &p, int &nextNodeIdx, const posterior_t &parentG)
{
    // #pragma HLS inline
    bool end_reached = false;
    update_extend(node,  p);

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
        #pragma HLS PIPELINE II=1
        parent[c] = newParent[c];
    }
}

bool allLabelsIdentical(const ap_byte_t counts[CLASS_COUNT], int &label)
{
    int uniqueLabels = 0;
    for(int c = 0; c < CLASS_COUNT; c++){
        //Checks for internal nodes the tabs, for leaf nodes the count
        if(counts[c] > 0){
            uniqueLabels++;
            label = c;
        }
    }
    return (uniqueLabels == 1);
}

bool process_active_node(Node_hbm &node, PageProperties &p, hls::stream<unit_interval> &rngStream, int &parentIdx, int &nextNodeIdx, IPage &page)
{
    
    rate_t e_cum[FEATURE_COUNT_TOTAL], rate = 0;

    calculate_e_values(node, p.input, e_cum, rate);
    splitT_t E;
    sample(E, rate, rngStream);

    bool endReached;
    if(rate != 0 && node.parentSplitTime + E < node.splittime){
        //Prepare for split
        rate_t rng_val = rngStream.read() * rate;
        p.setSplitProperties(node.idx(), determine_split_dimension(rng_val, e_cum), parentIdx, (node.parentSplitTime + E), rngStream.read(), false);
        endReached = true;
    }else{
        //Traverse
        parentIdx = node.idx();
        endReached = traverse(node, p, nextNodeIdx, p.parentG);
        update_parent_posterior(p.parentG, node.posteriorP);
        page[node.idx()] = nodeToRaw(node);
    }
    return endReached;
}

void process_pauzed_node(Node_hbm &node, PageProperties &p)
{
    update_extend(node, p);
    if(node.leaf()){
        if(node.counts[p.input.label] < 255){
            node.counts[p.input.label]++;
        }
    }
}


void sample(splitT_t &E, const rate_t &rate, hls::stream<unit_interval> &rngStream)
{
    if(rate != 0){
        ap_fixed<16, 4> randomValue = 1 - rngStream.read();
        ap_ufixed<16, 4, AP_TRN, AP_SAT> tmp = -hls::log(randomValue);
        ap_ufixed<16, 12,AP_TRN, AP_SAT> tmp2 = tmp/rate;
        E = tmp2;
    }
}