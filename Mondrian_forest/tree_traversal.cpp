#include "train.hpp"
#include <ap_fixed.h>
#include <hls_math.h>
#include <cwchar>
#include "converters.hpp"

void calculate_e_values(const Node_hbm &node, const input_vector &input, unit_interval e_l[FEATURE_COUNT_TOTAL], unit_interval e_u[FEATURE_COUNT_TOTAL], unit_interval e[FEATURE_COUNT_TOTAL], rate_t e_cum[FEATURE_COUNT_TOTAL], rate_t &rate);
int determine_split_dimension(const rate_t &rngValue, rate_t e_cum[FEATURE_COUNT_TOTAL]);
bool traverse(Node_hbm &node, PageProperties &p, unit_interval e_l[FEATURE_COUNT_TOTAL], unit_interval e_u[FEATURE_COUNT_TOTAL], int &nextNodeIdx);

void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &rngStream, hls::stream_of_blocks<IPage> &pageOut)
{
    
    if(!pageIn.empty()){
        unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL], e[FEATURE_COUNT_TOTAL];
        rate_t e_cum[FEATURE_COUNT_TOTAL];
        IPage localPage;
        //hls::write_lock<IPage> localPage(pageOut);
        PageProperties p;

        read_page(localPage, p, pageIn);
        int nextNodeIdx = 0;
        
        bool endReached = false;
        int parentIdx = 0;
        rate_t rate;
        //Traverse down the page
        tree_loop: while(!endReached){
            Node_hbm node(rawToNode(localPage[nextNodeIdx]));
            rate = 0;
            calculate_e_values(node, p.input, e_l, e_u, e, e_cum, rate);
            //splitT_t E = -std::log(1.0 - rngStream.read().to_float()) / rate.to_float(); //TODO: change from log to hls::log
            splitT_t E = -hls::log(1 - rngStream.read()) / rate;
            if(rate != 0 && node.parentSplitTime + E < node.splittime){
                //Prepare for split
                rate_t rng_val = rngStream.read() * rate;
                p.setSplitProperties(node.idx(), determine_split_dimension(rng_val, e_cum), parentIdx, (node.parentSplitTime + E), rngStream.read());
                endReached = true;
            }else{
                //Traverse
                parentIdx = node.idx();
                endReached = traverse(node, p, e_l, e_u, nextNodeIdx);
                localPage[node.idx()] = nodeToRaw(node);
            }
        }
        //localPage[MAX_NODES_PER_PAGE] = propertiesToRaw(p);
        write_page(localPage, p, pageOut);
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

bool traverse(Node_hbm &node, PageProperties &p, unit_interval e_l[FEATURE_COUNT_TOTAL], unit_interval e_u[FEATURE_COUNT_TOTAL], int &nextNodeIdx)
{
    // #pragma HLS inline
    bool end_reached = false;
    update_bounds: for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        #pragma HLS PIPELINE II=1
        node.lowerBound[d] = (e_l[d] !=0) ? p.input.feature[d] : node.lowerBound[d];
        node.upperBound[d] = (e_u[d] !=0) ? p.input.feature[d] : node.upperBound[d];
    }

    if(node.leaf()){
        ++node.labelCount;
        const ap_ufixed<32, 0> devisor = 1.0/node.labelCount;
        update_distribution: for(int i = 0; i < CLASS_COUNT; i++){
            #pragma HLS PIPELINE II=1
            node.classDistribution[i] = (node.classDistribution[i] * (node.labelCount - 1) + (p.input.label == i)) * devisor;
        }
        //Store changes to node
        end_reached = true;
    }else{
        //Traverse
        const ChildNode &child = (p.input.feature[node.feature] <= node.threshold) ? node.leftChild : node.rightChild;
        if (!child.isPage()) {
            nextNodeIdx = child.id();
        } else {
            p.nextPageIdx = child.id();
            p.needNewPage = true;
            end_reached = true;
        }
    }
    return end_reached;
}
