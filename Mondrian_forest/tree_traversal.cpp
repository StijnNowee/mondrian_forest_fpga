#include "train.hpp"

void calculate_e_values(Node_hbm &node, input_vector &input, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], float (&e)[FEATURE_COUNT_TOTAL], float (&e_cum)[FEATURE_COUNT_TOTAL], rate_t &rate);
int determine_split_dimension(float rngValue, float (&e_cum)[FEATURE_COUNT_TOTAL]);
bool traverse(node_converter &current, PageProperties &p, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], hls::write_lock<IPage> &out);

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

        auto p = convertRawToProperties(in[MAX_NODES_PER_PAGE]);
        node_converter current(out[0]);
        
        bool endReached = false;
        int parentIdx = 0;
        //Traverse down the page
        unit_interval rng_value = traversalRNGStream.read();
        tree_loop: for(int n = 0; n < MAX_DEPTH; n++){
            #pragma HLS PIPELINE OFF
            if(!endReached){
                rate_t rate = 0;
                calculate_e_values(current.node, p.input, e_l, e_u, e, e_cum, rate);
                float E = -std::log(1 - static_cast<float>(0.9)) / static_cast<float>(rate); //TODO: change from log to hls::log

                if(current.node.parentSplitTime + E < current.node.splittime){
                    //Prepare for split
                    float rng_val = rng_value * rate;
                    p.setSplitProperties(current.node.idx, determine_split_dimension(rng_val, e_cum), parentIdx, current.node.parentSplitTime + E);
                    endReached = true;
                }else{
                    //Traverse
                    parentIdx = current.node.idx;
                    endReached = traverse(current, p, e_l, e_u, out);
                }
            }
        }
        out[MAX_NODES_PER_PAGE] = convertPropertiesToRaw(p);
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

bool traverse(node_converter &current, PageProperties &p, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], hls::write_lock<IPage> &out)
{
    #pragma inline

    update_bounds: for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        current.node.lowerBound[d] = (e_l[d] !=0) ? p.input.feature[d] : current.node.lowerBound[d];
        current.node.upperBound[d] = (e_u[d] !=0) ? p.input.feature[d] : current.node.upperBound[d];
    }
    
    //Store changes to node
    out[current.node.idx] = current.raw;

    if(current.node.leaf){
        return true;
    }else{
        //Traverse
        ChildNode &child = (p.input.feature[current.node.feature] <= current.node.threshold) ? current.node.leftChild : current.node.rightChild;
        if (!child.isPage) {
            current.raw = out[child.id];
            return false;
        } else {
            p.nextPageIdx = child.id;
            return true;
        }
    }
}