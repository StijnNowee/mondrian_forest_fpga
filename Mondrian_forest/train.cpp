#include "train.hpp"
#include <cstring>


// void read_internal_page(Page pageIn, Page pageOut)
// {

// }


//TODO: CREATE FORLOOP, read continuously from newFeatureStream and feedbackStream. (needs stream of blocks)
void pre_fetcher(hls::stream<input_vector> &newFeatureStream, hls::stream<FetchRequest> &feedbackStream, Page pageOut, hls::stream<PageProperties> &traversalControl, const Page *pagePool)
{
        if(!feedbackStream.empty()){
            std::cout << "Feedback valid" << std::endl;
            
            FetchRequest request = feedbackStream.read();
            //burst_read_page(request.pageIdx, request.input, pagePool, pageOut);
            //request.valid = false;
        }
        if(!newFeatureStream.empty()){
            std::cout << "Prefetch page" << std::endl;

            auto newFeature = newFeatureStream.read();
            PageProperties p = {.input = {newFeature}, .pageIdx=0};
            
            bool invalidFound = false;
            for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
                pageOut[i] = pagePool[0][i];
                
                Node_hbm node;
                memcpy(&node, &pageOut[i], sizeof(Node_hbm));
                if(!invalidFound && !node.valid){
                    p.freeNodeIdx = i;
                    invalidFound = true;
                }
            }
            traversalControl.write(p);
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

void tree_traversal(const Page pageIn, hls::stream<unit_interval> &traversalRNGStream, Page pageOut, hls::stream<PageProperties> &control, hls::stream<PageProperties> &splitterControl)
{
    if(!control.empty() && !traversalRNGStream.empty()){
        std::cout << "Traverse" << std::endl;

        PageProperties p = control.read();
        for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
            pageOut[i] = pageIn[i];
        }

        Node_hbm node;
        memcpy(&node, &pageIn[0], sizeof(Node_hbm));

        unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
        float e[FEATURE_COUNT_TOTAL];
        bool endReached = false;
        
        for(int n = 0; n < MAX_PAGE_DEPTH; n++){
        //#pragma HLS PIPELINE OFF
            if(!endReached){
                rate rate = 0;
                for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                    #pragma HLS PIPELINE
                    e_l[d] = (node.lowerBound[d] > p.input.feature[d]) ? static_cast<unit_interval>(node.lowerBound[d] - p.input.feature[d]) : unit_interval(0);
                    e_u[d] = (p.input.feature[d] > node.upperBound[d]) ? static_cast<unit_interval>(p.input.feature[d] - node.upperBound[d]) : unit_interval(0);
                    e[d] = e_l[d] + e_u[d];
                    rate += e_l[d] + e_u[d];
                }
                float E = -std::log(1 - static_cast<float>(traversalRNGStream.read())) / static_cast<float>(rate); //TODO: change from log to hls::log
                if(node.parentSplitTime + E < node.splittime){
                    float rng_val = traversalRNGStream.read() * rate;
                    float total = 0;
                    for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                        total += e[d];
                        if(rng_val <= total){
                            p.split.dimension = d;
                            break;
                        }
                    }
                    p.split.split = true;
                    p.split.nodeIdx = node.idx;
                    p.split.parentIdx = node.parentIdx;
                    p.split.newSplitTime = node.parentSplitTime + E;
                    endReached = true;
                }else{
                    for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: not very efficient
                        if(e_l[d] != 0){
                            node.lowerBound[d] = p.input.feature[d];
                        }
                        if(e_u[d] !=0){
                            node.upperBound[d] = p.input.feature[d];
                        }
                    }
                    //Store changes to node
                    memcpy(&pageOut[node.idx], &node, sizeof(Node_hbm));

                    if(node.leaf){
                        endReached = true;
                    }else{
                        //Traverse
                        ChildNode child = (p.input.feature[node.feature] <= node.threshold) ? node.leftChild : node.rightChild;
                        if (!child.isPage) {
                            memcpy(&node, &pageIn[child.nodeIdx], sizeof(Node_hbm));
                        } else {
                            p.nextPageIdx = child.nodeIdx;
                            endReached = true;
                        }
                    }
                }
            }
        }
        splitterControl.write(p);
    }
}

void splitter(Page pageIn, hls::stream<unit_interval> &splitterRNGStream, Page pageOut, hls::stream<PageProperties> &control, hls::stream<PageProperties> &saveControl)
{
    if(!control.empty() && !splitterRNGStream.empty()){
        
        PageProperties p = control.read();
        for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            pageOut[i] = pageIn[i];
        }

        if(p.split.split){
            std::cout << "Split" << std::endl;
            unit_interval upperBound, lowerBound;
            Node_hbm currentNode, parentNode, newNode, newSibbling;
            memcpy(&currentNode, &pageOut[p.split.nodeIdx], sizeof(Node_hbm));
            memcpy(&parentNode, &pageOut[p.split.parentIdx], sizeof(Node_hbm));
            //Optimisable
            if(p.input.feature[p.split.dimension] > currentNode.upperBound[p.split.dimension]){
                lowerBound = currentNode.upperBound[p.split.dimension];
                upperBound = p.input.feature[p.split.dimension];
            }else{
                lowerBound = p.input.feature[p.split.dimension];
                upperBound = currentNode.lowerBound[p.split.dimension];
            }
            //SplitLocation
            if(currentNode.idx == 0){
                //new rootnode
                currentNode.idx = p.freeNodeIdx;
                newNode.idx = 0;
                Node_hbm child;
                memcpy(&child, &pageOut[currentNode.leftChild.nodeIdx], sizeof(Node_hbm)); //CHANGE FOR PAGE (MAYBE POINTER/REFERENCE?)
                child.parentIdx = currentNode.idx;
                memcpy(&pageOut[child.idx], &child, sizeof(Node_hbm));

                memcpy(&child, &pageOut[currentNode.rightChild.nodeIdx], sizeof(Node_hbm));
                child.parentIdx = currentNode.idx;
                memcpy(&pageOut[child.idx], &child, sizeof(Node_hbm));
            }else{
                newNode.idx = p.freeNodeIdx;
            }

            newNode.threshold = lowerBound + splitterRNGStream.read() * (upperBound - lowerBound);
            newNode.feature = p.split.dimension;
            newNode.splittime = p.split.newSplitTime;
            newNode.parentSplitTime = currentNode.parentSplitTime;
            newNode.valid = true;
            newNode.leaf = false;

            //New lower and upper bounds
            for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                #pragma HLS PIPELINE
                newNode.lowerBound[d] = (currentNode.lowerBound[d] > p.input.feature[d]) ? static_cast<unit_interval>(p.input.feature[d]) : currentNode.lowerBound[d];
                newNode.upperBound[d] = (p.input.feature[d] > currentNode.upperBound[d]) ? static_cast<unit_interval>(p.input.feature[d]) : currentNode.upperBound[d];
            }

            newSibbling.leaf = true;
            newSibbling.feature = p.input.label;
            newSibbling.idx = p.freeNodeIdx + 1;
            newSibbling.valid = true;
            newSibbling.splittime = std::numeric_limits<float>::max();
            newSibbling.parentSplitTime = newNode.splittime;
            newSibbling.lowerBound[0] = p.input.feature[0];
            newSibbling.lowerBound[1] = p.input.feature[1];
            newSibbling.upperBound[0] = p.input.feature[0];
            newSibbling.upperBound[1] = p.input.feature[1];

            if(p.input.feature[p.split.dimension] <= newNode.threshold){
                newNode.leftChild = ChildNode{.isPage = false, .nodeIdx = newSibbling.idx};
                newNode.rightChild = ChildNode{.isPage = false, .nodeIdx = currentNode.idx};
            }else{
                newNode.leftChild = ChildNode{.isPage = false, .nodeIdx = currentNode.idx};
                newNode.rightChild = ChildNode{.isPage = false, .nodeIdx = newSibbling.idx};
            };

            //Update connections of other nodes
            currentNode.parentIdx = newNode.idx;
            currentNode.parentSplitTime = newNode.splittime;
            if(parentNode.leftChild.nodeIdx == currentNode.idx){
                parentNode.leftChild.nodeIdx = newNode.idx;
            }else{
                parentNode.rightChild.nodeIdx = newNode.idx;
            }

            //Write new node
            memcpy(&pageOut[currentNode.idx], &currentNode, sizeof(Node_hbm));
            if(p.split.nodeIdx != p.split.parentIdx){
                memcpy(&pageOut[parentNode.idx], &parentNode, sizeof(Node_hbm));
            }
            memcpy(&pageOut[newNode.idx], &newNode, sizeof(Node_hbm));
            memcpy(&pageOut[newSibbling.idx], &newSibbling, sizeof(Node_hbm));
        }
        saveControl.write(p);
    }
}

void save(Page pageIn, hls::stream<FetchRequest> &feedbackStream, hls::stream<PageProperties> &control, Page *pagePool) //
{
    if(!control.empty()){
        PageProperties p = control.read();

        for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            pagePool[p.pageIdx][i] = pageIn[i];
        }

        if(p.nextPageIdx != 0){
            feedbackStream.write(FetchRequest {.input = p.input, .pageIdx = p.nextPageIdx, .valid = false});
        }
    }
}