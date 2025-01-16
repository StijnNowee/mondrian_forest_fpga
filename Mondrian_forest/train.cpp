#include "train.hpp"
#include <cstring>


// void read_internal_page(Page pageIn, Page pageOut)
// {

// }
void setChild(ChildNode &child, bool isPage, int nodeIdx)
{
    #pragma HLS INLINE
    child.isPage = isPage;
    child.nodeIdx = nodeIdx;
}

//TODO: CREATE FORLOOP, read continuously from newFeatureStream and feedbackStream. (needs stream of blocks)
void pre_fetcher(hls::stream<input_vector> &newFeatureStream, hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, int size)
{
    bool tree_done[TREES_PER_BANK];

    for(int iter = 0; iter < size; iter++){
        if(!feedbackStream.empty()){
            std::cout << "Feedback valid" << std::endl;
            
            FetchRequest request = feedbackStream.read();
            tree_done[request.treeID] = request.done;

            //burst_read_page(request.pageIdx, request.input, pagePool, pageOut);
            //request.valid = false;
        }
        if(!newFeatureStream.empty()){
            std::cout << "Prefetch page" << std::endl;

            auto newFeature = newFeatureStream.read();
            p_converter p_conv;
            p_conv.p = {.input = {newFeature}, .pageIdx=0};
            
            bool invalidFound = false;

            hls::write_lock<IPage> out(pageOut);
            for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
                out[i] = pagePool[0][i];
                
                node_converter converter;
                converter.raw = out[i];
                
                if(!invalidFound && !converter.node.valid){
                    p_conv.p.freeNodeIdx = i;
                    invalidFound = true;
                }
            }
            out[MAX_NODES_PER_PAGE] = p_conv.raw;
        }
    }
}


// void burst_read_page(int pageIdx, const input_vector &input, volatile const Page *pagePool, Page pageOut)
// {
//     bool invalidFound = false;
//     // volatile const Page& page = pagePool[pageIdx];

//     //memcpy(&pageStream, (const Page*)pagePool[pageIdx], sizeof(Page));
//     //memcpy(&pageStream, (const Page*)pagePool, MAX_NODES_PER_PAGE * sizeof(Node_hbm));
//     for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
//         pageOut[i] = pagePool[pageIdx][i];
        
//         Node_hbm node;
//         memcpy(&node, &pageOut[i], sizeof(Node_hbm));
//         if(!invalidFound && !node.valid){
//             //p.freeNodeIdx = i;
//             invalidFound = true;
//         }
//     }
// }

void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut, int size)
{
    //if(!control.empty() && !traversalRNGStream.empty()){
    for(int iter = 0; iter < size; iter++){
        std::cout << "Traverse" << std::endl;

        hls::read_lock<IPage> in(pageIn);
        hls::write_lock<IPage> out(pageOut);
        
        for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
            out[i] = in[i];
        }
        p_converter p_conv;
        p_conv.raw = in[MAX_NODES_PER_PAGE];

        node_converter converter;
        converter.raw = out[0];

        unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
        float e[FEATURE_COUNT_TOTAL];
        bool endReached = false;
        
        for(int n = 0; n < MAX_PAGE_DEPTH; n++){
        //#pragma HLS PIPELINE OFF
            if(!endReached){
                rate rate = 0;
                for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                    #pragma HLS PIPELINE
                    e_l[d] = (converter.node.lowerBound[d] > p_conv.p.input.feature[d]) ? static_cast<unit_interval>(converter.node.lowerBound[d] - p_conv.p.input.feature[d]) : unit_interval(0);
                    e_u[d] = (p_conv.p.input.feature[d] > converter.node.upperBound[d]) ? static_cast<unit_interval>(p_conv.p.input.feature[d] - converter.node.upperBound[d]) : unit_interval(0);
                    e[d] = e_l[d] + e_u[d];
                    rate += e_l[d] + e_u[d];
                }
                float E = -std::log(1 - static_cast<float>(traversalRNGStream.read())) / static_cast<float>(rate); //TODO: change from log to hls::log
                if(converter.node.parentSplitTime + E < converter.node.splittime){
                    float rng_val = traversalRNGStream.read() * rate;
                    float total = 0;
                    for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                        total += e[d];
                        if(rng_val <= total){
                            p_conv.p.split.dimension = d;
                            break;
                        }
                    }
                    p_conv.p.split.split = true;
                    p_conv.p.split.nodeIdx = converter.node.idx;
                    p_conv.p.split.parentIdx = converter.node.parentIdx;
                    p_conv.p.split.newSplitTime = converter.node.parentSplitTime + E;
                    endReached = true;
                }else{
                    for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: not very efficient
                        if(e_l[d] != 0){
                            converter.node.lowerBound[d] = p_conv.p.input.feature[d];
                        }
                        if(e_u[d] !=0){
                            converter.node.upperBound[d] = p_conv.p.input.feature[d];
                        }
                    }
                    //Store changes to node
                    out[converter.node.idx] = converter.raw;

                    if(converter.node.leaf){
                        endReached = true;
                    }else{
                        //Traverse
                        ChildNode child = (p_conv.p.input.feature[converter.node.feature] <= converter.node.threshold) ? converter.node.leftChild : converter.node.rightChild;
                        if (!child.isPage) {
                            converter.raw = out[child.nodeIdx];
                        } else {
                            p_conv.p.nextPageIdx = child.nodeIdx;
                            endReached = true;
                        }
                    }
                }
            }
        }
        out[MAX_NODES_PER_PAGE] = p_conv.raw;
    }
}

void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut, int size)
{
    for(int iter = 0; iter < size; iter++){
        
        hls::read_lock<IPage> in(pageIn);
        hls::write_lock<IPage> out(pageOut);
        for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            out[i] = in[i];
        }
        p_converter p_conv;
        p_conv.raw = in[MAX_NODES_PER_PAGE];

        if(p_conv.p.split.split){
            std::cout << "Split" << std::endl;
            unit_interval upperBound, lowerBound;
            
            node_converter current, parent, split, newSibbling;
            current.raw = out[p_conv.p.split.nodeIdx];
            parent.raw = out[p_conv.p.split.parentIdx  ];
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
                Node_hbm child;
                memcpy(&child, &out[current.node.leftChild.nodeIdx], sizeof(Node_hbm)); //CHANGE FOR PAGE (MAYBE POINTER/REFERENCE?)
                child.parentIdx = current.node.idx;
                memcpy(&out[child.idx], &child, sizeof(Node_hbm));

                memcpy(&child, &out[current.node.rightChild.nodeIdx], sizeof(Node_hbm));
                child.parentIdx = current.node.idx;
                memcpy(&out[child.idx], &child, sizeof(Node_hbm));
            }else{
                split.node.idx = p_conv.p.freeNodeIdx;
            }

            split.node.threshold = lowerBound + splitterRNGStream.read() * (upperBound - lowerBound);
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

void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool, int size) //
{
    for(int iter = 0; iter < size; iter++){
        hls::read_lock<IPage> in(pageIn);

        p_converter p_conv;
        p_conv.raw = in[MAX_NODES_PER_PAGE];

        for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            pagePool[p_conv.p.pageIdx][i] = in[i];
        }

        feedbackStream.write(FetchRequest {.input = p_conv.p.input, .pageIdx = p_conv.p.nextPageIdx, .done = true});
    }
}