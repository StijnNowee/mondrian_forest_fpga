#include "train.hpp"

void calculate_e_values(Node_hbm &node, input_vector &input, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], float (&e)[FEATURE_COUNT_TOTAL], float (&e_cum)[FEATURE_COUNT_TOTAL], rate_t &rate);
int determine_split_dimension(float rngValue, float (&e_cum)[FEATURE_COUNT_TOTAL]);
//bool traverse(Node_hbm &node, PageProperties &p, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], hls::write_lock<IPage> &out);

void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut, const int loopCount, hls::stream<bool> &treeDoneStream)
{
    unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
    float e[FEATURE_COUNT_TOTAL], e_cum[FEATURE_COUNT_TOTAL];

    main_loop: for(int iter = 0; iter < loopCount;){
        if(!pageIn.empty()){
        hls::read_lock<IPage> in(pageIn);
        hls::write_lock<IPage> out(pageOut);
        
        //Copy input
        save_to_output: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            out[i] = in[i];
        }

        auto p = convertProperties(in[MAX_NODES_PER_PAGE]);
        Node_hbm node;
        convertRawToNode(out[0], node);
        
        bool endReached = false;
        int parentIdx = 0;
        //Traverse down the page
        tree_loop: for(int n = 0; n < MAX_DEPTH; n++){
            #pragma HLS PIPELINE OFF
            if(!endReached){
                rate_t rate = 0;
                calculate_e_values(node, p.input, e_l, e_u, e, e_cum, rate);
                splitT_t E = -std::log(1.0 - traversalRNGStream.read().to_float()) / rate.to_float(); //TODO: change from log to hls::log

                if(rate != 0 && node.parentSplitTime + E < node.splittime){
                    //Prepare for split
                    rate_t rng_val = traversalRNGStream.read() * rate;
                    p.setSplitProperties(node.idx, determine_split_dimension(rng_val, e_cum), parentIdx, (node.parentSplitTime + E));
                    endReached = true;
                }else{
                    //Traverse
                    parentIdx = node.idx;
                    //endReached = traverse(node, p, e_l, e_u, out);
                    update_bounds: for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                        node.lowerBound[d] = (e_l[d] !=0) ? p.input.feature[d] : node.lowerBound[d];
                        node.upperBound[d] = (e_u[d] !=0) ? p.input.feature[d] : node.upperBound[d];
                    }
                    
                    //Store changes to node
                    //out[node.idx] = convertNodeToRaw(node);
                    

                    if(node.leaf){
                        endReached = true;
                    }else{
                        //Traverse
                        ChildNode child = (p.input.feature[node.feature] <= node.threshold) ? node.leftChild : node.rightChild;
                        if (!child.isPage) {
                            convertRawToNode(out[child.id], node);
                            endReached = false;
                        } else {
                            p.nextPageIdx = child.id;
                            p.needNewPage = true;
                            endReached = true;
                        }
                    }
                }
            }
        }
        out[MAX_NODES_PER_PAGE] = convertProperties(p);
        }
        #if(defined __SYNTHESIS__)
             if(!treeDoneStream.empty()){
                treeDoneStream.read();
                iter++;
            }
        #else
            iter++;
        #endif
    }
}

void calculate_e_values(Node_hbm &node, input_vector &input, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], float (&e)[FEATURE_COUNT_TOTAL], float (&e_cum)[FEATURE_COUNT_TOTAL], rate_t &rate)
{
    #pragma HLS inline
    calculate_e_values: for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        e_l[d] = (node.lowerBound[d] > input.feature[d]) ? static_cast<unit_interval>(node.lowerBound[d] - input.feature[d]) : unit_interval(0);
        e_u[d] = (input.feature[d] > node.upperBound[d]) ? static_cast<unit_interval>(input.feature[d] - node.upperBound[d]) : unit_interval(0);
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

// bool traverse(Node_hbm &node, PageProperties &p, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], hls::write_lock<IPage> &out)
// {
//     //#pragma HLS inline

//     // update_bounds: for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
//     //     node.lowerBound[d] = (e_l[d] !=0) ? p.input.feature[d] : node.lowerBound[d];
//     //     node.upperBound[d] = (e_u[d] !=0) ? p.input.feature[d] : node.upperBound[d];
//     // }
    
//     // //Store changes to node
//     // out[node.idx] = convertNodeToRaw(node);
    

//     // if(node.leaf){
//     //     return true;
//     // }else{
//     //     //Traverse
//     //     ChildNode child = (p.input.feature[node.feature] <= node.threshold) ? node.leftChild : node.rightChild;
//     //     if (!child.isPage) {
//     //         node = convertRawToNode(out[child.id]);
//     //         return false;
//     //     } else {
//     //         p.nextPageIdx = child.id;
//     //         p.needNewPage = true;
//     //         return true;
//     //     }
//     // }
// }