#include "train.hpp"
#include <cwchar>

void calculate_e_values(Node_hbm &node, input_vector &input, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], float (&e)[FEATURE_COUNT_TOTAL], float (&e_cum)[FEATURE_COUNT_TOTAL], rate_t &rate);
int determine_split_dimension(float rngValue, float (&e_cum)[FEATURE_COUNT_TOTAL]);
bool traverse(Node_hbm &node, PageProperties &p, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], IPage &localPage);

void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut)
{
    if(!pageIn.empty()){
    unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
    float e[FEATURE_COUNT_TOTAL], e_cum[FEATURE_COUNT_TOTAL];
    IPage localPage;
    PageProperties p;

    read_page(localPage, p, pageIn);
    Node_hbm node;
    convertRawToNode(localPage[0], node);
    // memcpy(&node, &in[0], sizeof(Node_hbm));
    
    bool endReached = false;
    int parentIdx = 0;
    //Traverse down the page
    tree_loop: for(int n = 0; n < MAX_DEPTH; n++){
        #pragma HLS PIPELINE OFF
        if(!endReached){
            rate_t rate = 0;
            calculate_e_values(node, p.input, e_l, e_u, e, e_cum, rate);
            splitT_t E = -std::log(1.0 - unit_interval(0.9).to_float()) / rate.to_float(); //TODO: change from log to hls::log

            if(rate != 0 && node.parentSplitTime + E < node.splittime){
                //Prepare for split
                rate_t rng_val = unit_interval(0.9) * rate;
                p.setSplitProperties(node.idx, determine_split_dimension(rng_val, e_cum), parentIdx, (node.parentSplitTime + E));
                endReached = true;
            }else{
                //Traverse
                parentIdx = node.idx;
                endReached = traverse(node, p, e_l, e_u, localPage);
            }
        }
    }
    write_page(localPage, p, pageOut);
    }
}


void calculate_e_values(Node_hbm &node, input_vector &input, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], float (&e)[FEATURE_COUNT_TOTAL], float (&e_cum)[FEATURE_COUNT_TOTAL], rate_t &rate)
{
    #pragma HLS inline
    calculate_e_values: for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        e_l[d] = (node.lowerBound[d] > input.feature[d]) ? unit_interval(node.lowerBound[d] - input.feature[d]) : unit_interval(0);
        e_u[d] = (input.feature[d] > node.upperBound[d]) ? unit_interval(input.feature[d] - node.upperBound[d]) : unit_interval(0);
        e[d] = e_l[d] + e_u[d];
        rate += e_l[d] + e_u[d];
        e_cum[d] = rate;
    }
}

int determine_split_dimension(float rngValue, float (&e_cum)[FEATURE_COUNT_TOTAL])
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

bool traverse(Node_hbm &node, PageProperties &p, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], IPage &localPage)
{
//     //#pragma HLS inline

    update_bounds: for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        node.lowerBound[d] = (e_l[d] !=0) ? p.input.feature[d] : node.lowerBound[d];
        node.upperBound[d] = (e_u[d] !=0) ? p.input.feature[d] : node.upperBound[d];
    }

    if(node.leaf){
        ++node.labelCount;
        update_distribution: for(int i = 0; i < CLASS_COUNT; i++){
            node.classDistribution[i] = (node.classDistribution[i] * (node.labelCount - 1) + (p.input.label == i)) / node.labelCount;
        }
        //Store changes to node
        convertNodeToRaw(node, localPage[node.idx]);
        // memcpy(&out[node.idx], &node, sizeof(Node_hbm));
        return true;
    }else{
        convertNodeToRaw(node, localPage[node.idx]);
        // memcpy(&out[node.idx], &node, sizeof(Node_hbm));

        //Traverse
        ChildNode child = (p.input.feature[node.feature] <= node.threshold) ? node.leftChild : node.rightChild;
        if (!child.isPage) {
            // memcpy(&node, &out[child.id], sizeof(Node_hbm));
            convertRawToNode(localPage[child.id], node);
            return false;
        } else {
            p.nextPageIdx = child.id;
            p.needNewPage = true;
            return true;
        }
    }
}
