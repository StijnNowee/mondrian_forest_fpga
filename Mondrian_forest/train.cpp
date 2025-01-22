#include "train.hpp"
#include <cstring>
#include <etc/autopilot_ssdm_op.h>

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const int pageIdx, const Page *pagePool, bool feedback);

void calculate_e_values(Node_hbm &node, input_vector &input, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], float (&e)[FEATURE_COUNT_TOTAL], float (&e_cum)[FEATURE_COUNT_TOTAL], rate_t &rate);
int determine_split_dimension(float rngValue, float (&e_cum)[FEATURE_COUNT_TOTAL]);
bool traverse(node_converter &current, PageProperties &p, unit_interval (&e_l)[FEATURE_COUNT_TOTAL], unit_interval (&e_u)[FEATURE_COUNT_TOTAL], hls::write_lock<IPage> &out);
void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, hls::write_lock<IPage> &out, const int freeNodeIdx);
void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream, bool rootPage);

void feature_distributor(hls::stream<input_vector> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], int size)
{
    for(int i = 0; i < size; i++){
        auto newFeature = newFeatureStream.read();
        for(int t = 0; t < TREES_PER_BANK; t++){
            splitFeatureStream[t].write(newFeature);
        }
    }
}

//TODO overhaul, use fetchRequests to handle this
void pre_fetcher(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, const int loopCount)
{
    //Initialise status
    TreeStatus status[TREES_PER_BANK];
    int scheduled[TREES_PER_BANK];
    int nrScheduled = 0;
    for (int i = 0; i < TREES_PER_BANK; i++) {
        status[i] = IDLE;
        scheduled[i] = TREES_PER_BANK + 1;
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
            check_idle_trees: for(int t = 0; t < TREES_PER_BANK; t++){
                if(status[t] == IDLE && !splitFeatureStream[t].empty()){
                    scheduled[nrScheduled++] = t;
                    #if(defined __SYNTHESIS__)
                        status[t] = PROCESSING;
                    #else
                        iter++;
                    #endif
                }
            }
            if(nrScheduled > 0){
                scheduled_loop: for(int st = 0; st < nrScheduled; ++st){
                    #pragma HLS PIPELINE II=MAX_NODES_PER_PAGE
                    auto input = splitFeatureStream[scheduled[st]].read();
                    //Fetch the first page for the tree
                    burst_read_page(pageOut, input, scheduled[st], 0, pagePool, false);
                }
                nrScheduled = 0;
            }
        }
    }
}

void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval, 100> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut, const int loopCount)
{
    unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
    float e[FEATURE_COUNT_TOTAL], e_cum[FEATURE_COUNT_TOTAL];

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
        int parentIdx = 0;
        //Traverse down the page
        tree_loop: for(int n = 0; n < MAX_DEPTH; n++){
            #pragma HLS PIPELINE OFF
            if(!endReached){
                rate_t rate = 0;
                calculate_e_values(current.node, p.input, e_l, e_u, e, e_cum, rate);
                float E = -std::log(1 - static_cast<float>(0.9)) / static_cast<float>(rate); //TODO: change from log to hls::log

                if(current.node.parentSplitTime + E < current.node.splittime){
                    //Prepare for split
                    float rng_val = unit_interval(0.9) * rate;
                    p.setSplitProperties(current.node.idx, determine_split_dimension(rng_val, e_cum), parentIdx, current.node.parentSplitTime + E);
                    endReached = true;
                }else{
                    //Traverse
                    parentIdx = current.node.idx;
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
        //Copy input
        hls::read_lock<IPage> in(pageIn);
        hls::write_lock<IPage> out(pageOut);
        save_to_output: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            out[i] = in[i];
        }

        p_converter p_conv(in[MAX_NODES_PER_PAGE]);

        if(p_conv.p.split.enabled){
            
            auto &sp = p_conv.p.split;
            node_converter current(out[sp.nodeIdx]);

            auto featureValue = p_conv.p.input.feature[sp.dimension]; 
            unit_interval upperBound = current.node.lowerBound[sp.dimension], lowerBound = current.node.upperBound[sp.dimension]; //Intended
            //Seems strange but saves a operation
            if(featureValue > lowerBound){
                upperBound = featureValue;
            }else{
                lowerBound = featureValue;
            }

            //Create the two new nodes
            node_converter newNode(p_conv.p.split.dimension, 
                                p_conv.p.split.newSplitTime, 
                                current.node.parentSplitTime,
                                lowerBound + unit_interval(0.9) * (upperBound - lowerBound), 
                                false);

            assign_node_idx(current.node, newNode.node, out, p_conv.p.freeNodeIdx);

            node_converter newSibbling(p_conv.p.input.label, 
                                        std::numeric_limits<float>::max(),
                                        newNode.node.splittime, 
                                        0, 
                                        true, 
                                        p_conv.p.freeNodeIdx + 1);
            
            //New lower and upper bounds
            for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                #pragma HLS PIPELINE
                auto feature = p_conv.p.input.feature[d];

                newNode.node.lowerBound[d] = (current.node.lowerBound[d] > feature) ? feature : current.node.lowerBound[d];
                newNode.node.upperBound[d] = (feature > current.node.upperBound[d]) ? feature : current.node.upperBound[d];
                newSibbling.node.lowerBound[d] = feature;
                newSibbling.node.upperBound[d] = feature;
            }

            if(p_conv.p.input.feature[sp.dimension] <= newNode.node.threshold){
                newNode.node.leftChild = ChildNode(false, newSibbling.node.idx);
                newNode.node.rightChild = ChildNode(false, current.node.idx);
            }else{
                newNode.node.leftChild = ChildNode(false, current.node.idx);
                newNode.node.rightChild = ChildNode(false, newSibbling.node.idx);
            };

            node_converter parent(out[sp.parentIdx]);

            //Update connections of other nodes
            current.node.parentSplitTime = newNode.node.splittime;
            if(parent.node.leftChild.nodeIdx == current.node.idx){
                parent.node.leftChild.nodeIdx = newNode.node.idx;
            }else{
                parent.node.rightChild.nodeIdx = newNode.node.idx;
            }

            //Write new node
            out[current.node.idx] = current.raw;
            out[newNode.node.idx] = newNode.raw;
            out[newSibbling.node.idx] = newSibbling.raw;
            if(sp.nodeIdx != sp.parentIdx){
                out[parent.node.idx] = parent.raw;
            }
        }
        out[MAX_NODES_PER_PAGE] = p_conv.raw;
    }
}

void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool, const int loopCount) //
{
    main_loop: for(int iter = 0; iter < loopCount; iter++){
        hls::read_lock<IPage> in(pageIn);

        p_converter p_conv(in[MAX_NODES_PER_PAGE]);
        const int globalPageIdx = p_conv.p.treeID * MAX_PAGES_PER_TREE + p_conv.p.pageIdx;
        for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            #pragma HLS PIPELINE II=5
            pagePool[globalPageIdx][i] = in[i];
        }
        //Create new request
        auto request = FetchRequest {.input = p_conv.p.input, .pageIdx = p_conv.p.nextPageIdx, .treeID = p_conv.p.treeID,  .done = true};
        
        //Race condition blocker
        sendFeedback(request, feedbackStream, p_conv.p.pageIdx == 0);
    }
}

void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream, bool rootPage)
{
        //Race condition blocker
        if(rootPage && request.done){
            ap_wait_n(30);
        }
        feedbackStream.write(request);
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

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const int pageIdx, const Page *pagePool, bool feedback)
{
    #pragma HLS inline off
    p_converter p_conv(feature, pageIdx, treeID ,feedback);
    bool invalidFound = false;
    const int globalPageIdx = treeID * MAX_PAGES_PER_TREE + pageIdx;
    hls::write_lock<IPage> out(pageOut);
    for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        out[i] = pagePool[globalPageIdx][i];
        

    }
    
    node_converter node_cov;
    for(int i=0; i < MAX_NODES_PER_PAGE; i++){
        node_cov.raw = out[i];
        
        if(!invalidFound && !node_cov.node.valid){
            p_conv.p.freeNodeIdx = i;
            invalidFound = true;
        }
    }
    out[MAX_NODES_PER_PAGE] = p_conv.raw;
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
            current.raw = out[child.nodeIdx];
            return false;
        } else {
            p.nextPageIdx = child.nodeIdx;
            return true;
        }
    }
}

void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, hls::write_lock<IPage> &out, const int freeNodeIdx)
{
    if(currentNode.idx == 0){
        //New root node
        currentNode.idx = freeNodeIdx;
        newNode.idx = 0;

    }else{
        //Add node to array
        newNode.idx = freeNodeIdx;
    }
}