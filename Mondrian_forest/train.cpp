#include "train.hpp"
#include <cstring>
#include <etc/autopilot_ssdm_op.h>

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const int pageIdx, const Page *pagePool, bool feedback);

void calculate_e_values(Node_hbm &node, input_vector &input, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], float (&e)[FEATURE_COUNT_TOTAL], rate_t &rate);
int determine_split_dimension(float rngValue, float (&e)[FEATURE_COUNT_TOTAL]);
bool traverse(node_converter &current, PageProperties &p, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], hls::write_lock<IPage> &out);

void setChild(ChildNode &child, bool isPage, int nodeIdx)
{
    #pragma HLS INLINE
    child.isPage = isPage;
    child.nodeIdx = nodeIdx;
}

void feature_distributor(hls::stream<input_vector> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], int size)
{
    for(int i = 0; i < size; i++){
        auto newFeature = newFeatureStream.read();
        for(int t = 0; t < TREES_PER_BANK; t++){
            splitFeatureStream[t].write(newFeature);
        }
    }
}

void pre_fetcher(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, const int loopCount)
{
    //Initialise status
    TreeStatus status[TREES_PER_BANK];
    for (int i = 0; i < TREES_PER_BANK; i++) {
        status[i] = IDLE;
    }

    main_loop: for(int iter = 0; iter < loopCount;){
        // Prioritize processing feedback requests from the feedback stream.
        if(!feedbackStream.empty()){
            FetchRequest request = feedbackStream.read();
            if(request.done){
                //Tree finished processing
                status[request.treeID] = IDLE;
                iter++;
            }else{
                //Tree needs new page
                burst_read_page(pageOut, request.input, request.treeID, request.pageIdx, pagePool, true);
            }
        } else{
            // If no feedback, check for new input vectors for idle trees.
            for(int t = 0; t < TREES_PER_BANK; t++){
                if(status[t] == IDLE && !splitFeatureStream[t].empty()){
                    auto input = splitFeatureStream[t].read();

                    //Fetch the first page for the tree
                    burst_read_page(pageOut, input, t, 0, pagePool, false);
                    #if(defined __SYNTHESIS__)
                        status[t] = PROCESSING;
                    #else
                        iter++;
                    #endif
                }
            }
        }
    }
}

void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval, 100> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut, const int loopCount)
{
    unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
    float e[FEATURE_COUNT_TOTAL];

    main_loop: for(int iter = 0; iter < loopCount; iter++){

        hls::read_lock<IPage> in(pageIn);
        hls::write_lock<IPage> out(pageOut);
        
        //Copy input
        save_to_output: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            out[i] = in[i];
        }

        p_converter p_conv(in[MAX_NODES_PER_PAGE]);
        node_converter current(out[0]);
        auto &p = p_conv.p;
        
        bool endReached = false;
        //Traverse down the page
        tree_loop: for(int n = 0; n < MAX_DEPTH; n++){
            #pragma HLS PIPELINE OFF
            if(!endReached){
                rate_t rate = 0;
                calculate_e_values(current.node, p.input, e_l, e_u, e, rate);
                float E = -std::log(1 - static_cast<float>(0.9)) / static_cast<float>(rate); //TODO: change from log to hls::log

                if(current.node.parentSplitTime + E < current.node.splittime){
                    //Prepare for split
                    float rng_val = unit_interval(0.9) * rate;
                    p.setSplitProperties(current.node.idx, determine_split_dimension(rng_val, e), current.node.parentIdx, current.node.parentSplitTime + E);
                    endReached = true;
                }else{
                    //Traverse
                    endReached = traverse(current, p, e_l, e_u, out);
                }
            }
        }
        out[MAX_NODES_PER_PAGE] = p_conv.raw;
    }
}

void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval, 100> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut, const int loopCount)
{
    main_loop: for(int iter = 0; iter < loopCount; iter++){
        
        hls::read_lock<IPage> in(pageIn);
        hls::write_lock<IPage> out(pageOut);
        for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            out[i] = in[i];
        }
        p_converter p_conv;
        p_conv.raw = in[MAX_NODES_PER_PAGE];

        if(p_conv.p.split.enabled){
            unit_interval upperBound, lowerBound;
            
            node_converter current, parent, split, newSibbling;
            current.raw = out[p_conv.p.split.nodeIdx];
            parent.raw = out[p_conv.p.split.parentIdx];
            //Optimisable
            if(p_conv.p.input.feature[p_conv.p.split.dimension] > current.node.upperBound[p_conv.p.split.dimension]){
                lowerBound = current.node.upperBound[p_conv.p.split.dimension];
                upperBound = p_conv.p.input.feature[p_conv.p.split.dimension];
            }else{
                lowerBound = p_conv.p.input.feature[p_conv.p.split.dimension];
                upperBound = current.node.lowerBound[p_conv.p.split.dimension];
            }
            //SplitLocation
            if(current.node.idx == 0){
                //new rootnode
                current.node.idx = p_conv.p.freeNodeIdx;
                split.node.idx = 0;
                node_converter child_c;
                child_c.raw = out[current.node.leftChild.nodeIdx]; //CHANGE FOR PAGE (MAYBE POINTER/REFERENCE?)
                child_c.node.parentIdx = current.node.idx;
                out[child_c.node.idx] = child_c.raw;

                child_c.raw = out[current.node.rightChild.nodeIdx];
                child_c.node.parentIdx = current.node.idx;
                out[child_c.node.idx] = child_c.raw;
            }else{
                split.node.idx = p_conv.p.freeNodeIdx;
            }

            split.node.threshold = lowerBound + unit_interval(0.9) * (upperBound - lowerBound);
            split.node.feature = p_conv.p.split.dimension;
            split.node.splittime = p_conv.p.split.newSplitTime;
            split.node.parentSplitTime = current.node.parentSplitTime;
            split.node.valid = true;
            split.node.leaf = false;

            //New lower and upper bounds
            for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                #pragma HLS PIPELINE
                split.node.lowerBound[d] = (current.node.lowerBound[d] > p_conv.p.input.feature[d]) ? static_cast<unit_interval>(p_conv.p.input.feature[d]) : current.node.lowerBound[d];
                split.node.upperBound[d] = (p_conv.p.input.feature[d] > current.node.upperBound[d]) ? static_cast<unit_interval>(p_conv.p.input.feature[d]) : current.node.upperBound[d];
            }

            newSibbling.node.leaf = true;
            newSibbling.node.feature = p_conv.p.input.label;
            newSibbling.node.idx = p_conv.p.freeNodeIdx + 1;
            newSibbling.node.valid = true;
            newSibbling.node.splittime = std::numeric_limits<float>::max();
            newSibbling.node.parentSplitTime = split.node.splittime;
            newSibbling.node.lowerBound[0] = p_conv.p.input.feature[0];
            newSibbling.node.lowerBound[1] = p_conv.p.input.feature[1];
            newSibbling.node.upperBound[0] = p_conv.p.input.feature[0];
            newSibbling.node.upperBound[1] = p_conv.p.input.feature[1];

            if(p_conv.p.input.feature[p_conv.p.split.dimension] <= split.node.threshold){
                setChild(split.node.leftChild, false,newSibbling.node.idx);
                setChild(split.node.rightChild, false,current.node.idx);

                // newNode.leftChild = ChildNode(false, newSibbling.idx);
                // newNode.rightChild = ChildNode(false, currentNode.idx);
            }else{
                setChild(split.node.leftChild, false,current.node.idx);
                setChild(split.node.rightChild, false,newSibbling.node.idx);
            };

            //Update connections of other nodes
            current.node.parentIdx = split.node.idx;
            current.node.parentSplitTime = split.node.splittime;
            if(parent.node.leftChild.nodeIdx == current.node.idx){
                parent.node.leftChild.nodeIdx = split.node.idx;
            }else{
                parent.node.rightChild.nodeIdx = split.node.idx;
            }

            //Write new node
            out[current.node.idx] = current.raw;
            if(p_conv.p.split.nodeIdx != p_conv.p.split.parentIdx){
                out[parent.node.idx] = parent.raw;
            }
            out[split.node.idx] = split.raw;
            out[newSibbling.node.idx] = newSibbling.raw;
        }
        out[MAX_NODES_PER_PAGE] = p_conv.raw;
    }
}

void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool, const int loopCount) //
{
    main_loop: for(int iter = 0; iter < loopCount; iter++){
        hls::read_lock<IPage> in(pageIn);

        p_converter p_conv;
        p_conv.raw = in[MAX_NODES_PER_PAGE];

        for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            pagePool[p_conv.p.treeID*MAX_PAGES_PER_TREE + p_conv.p.pageIdx][i] = in[i];
        }
        auto request = FetchRequest {.input = p_conv.p.input, .pageIdx = p_conv.p.nextPageIdx, .treeID = p_conv.p.treeID,  .done = true};
        if(request.done && p_conv.p.pageIdx == 0){
            ap_wait_n(10);
        }
        feedbackStream.write(request); //Race condition
    }
}


void calculate_e_values(Node_hbm &node, input_vector &input, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], float (&e)[FEATURE_COUNT_TOTAL], rate_t &rate)
{
    #pragma HLS inline
    calculate_e_values: for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        e_l[d] = (node.lowerBound[d] > input.feature[d]) ? static_cast<unit_interval>(node.lowerBound[d] - input.feature[d]) : unit_interval(0);
        e_u[d] = (input.feature[d] > node.upperBound[d]) ? static_cast<unit_interval>(input.feature[d] - node.upperBound[d]) : unit_interval(0);
        e[d] = e_l[d] + e_u[d];
        rate += e_l[d] + e_u[d];
    }
}

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const int pageIdx, const Page *pagePool, bool feedback)
{
    #pragma HLS inline off
    p_converter p_conv(feature, pageIdx, treeID ,feedback);
    bool invalidFound = false;
    const int globalPageIdx = treeID * MAX_PAGES_PER_TREE + pageIdx;
    node_converter node_cov;
    hls::write_lock<IPage> out(pageOut);
    for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        out[i] = pagePool[globalPageIdx][i];
        
        node_cov.raw = out[i];
        
        if(!invalidFound && !node_cov.node.valid){
            p_conv.p.freeNodeIdx = i;
            invalidFound = true;
        }
    }
    out[MAX_NODES_PER_PAGE] = p_conv.raw;
}

int determine_split_dimension(float rngValue, float (&e)[FEATURE_COUNT_TOTAL])
{
    #pragma HLS inline
    float total = 0;
    int splitDimension = UNDEFINED_DIMENSION;
    split_dimension: for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        total += e[d];
        if(rngValue <= total && splitDimension == UNDEFINED_DIMENSION){
            splitDimension = d;
        }
    }
    return splitDimension;
}

bool traverse(node_converter &current, PageProperties &p, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], hls::write_lock<IPage> &out)
{
    #pragma inline
    update_bounds: for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: not very efficient
        if(e_l[d] != 0){
            current.node.lowerBound[d] = p.input.feature[d];
        }
        if(e_u[d] !=0){
            current.node.upperBound[d] = p.input.feature[d];
        }
    }
    
    //Store changes to node
    out[current.node.idx] = current.raw;

    if(current.node.leaf){
        return true;
    }else{
        //Traverse
        ChildNode child = (p.input.feature[current.node.feature] <= current.node.threshold) ? current.node.leftChild : current.node.rightChild;
        if (!child.isPage) {
            current.raw = out[child.nodeIdx];
            return false;
        } else {
            p.nextPageIdx = child.nodeIdx;
            return true;
        }
    }
}