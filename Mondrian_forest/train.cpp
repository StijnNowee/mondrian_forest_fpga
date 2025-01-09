#include "train.hpp"
void pre_fetcher_old(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool)
{
    #pragma HLS PIPELINE
    #pragma HLS INTERFACE s_axilite port=return

    if(!fetchRequestStream.empty()){
        std::cout << "Prefetch page" << std::endl;
        auto request = fetchRequestStream.read();
        hls::write_lock<IPage> b(pageOut);
        for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
            #pragma HLS PIPELINE
            Node_hbm node = pagePool[request.pageIdx][i];
            memcpy(&b[i], &node, sizeof(Node_hbm));
            std::cout << "in loop: " << node.idx << std::endl;
        }
        // auto node = pagePool[request.pageIdx][0];
        // std::cout << "original node: " << node.idx << std::endl;
        PageProperties p = {.input = request.input, .pageIdx=request.pageIdx};
        memcpy(&b[MAX_NODES_PER_PAGE], &p, sizeof(PageProperties));
    }
}

void pre_fetcher(hls::stream<input_vector> &newFeatureStream, hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool)
{
    if(!feedbackStream.empty()){
        std::cout << "Feedback valid" << std::endl;
        hls::write_lock<IPage> b(pageOut);
        FetchRequest request = feedbackStream.read();
        burst_read_page(request.pageIdx, request.input, pagePool, b);
        request.valid = false;
    }
    else if(!newFeatureStream.empty()){
        std::cout << "Prefetch page" << std::endl;
        auto newFeature = newFeatureStream.read();
        for(int j = 0; j < TREES_PER_BANK; j++){
            #pragma HLS PIPELINE OFF
            std::cout << "Lock" << std::endl;
            hls::write_lock<IPage> b(pageOut);
            burst_read_page(0, newFeature, pagePool, b);
        }
    }
}


void burst_read_page(int pageIdx, const input_vector &input, const Page *pagePool, hls::write_lock<IPage> &pageStream)
{
    bool invalidFound = false;
    PageProperties p = {.input = {input}, .pageIdx=pageIdx};

    for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        Node_hbm node = pagePool[pageIdx][i];
        if(!invalidFound && !node.valid){
            p.freeNodeIdx = i;
            invalidFound = true;
        }
        memcpy(&pageStream[i], &node, sizeof(Node_hbm));
    }
    
    memcpy(&pageStream[MAX_NODES_PER_PAGE], &p, sizeof(PageProperties));
}

void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut)
{
    #pragma HLS PIPELINE
    if(!pageIn.empty() && !traversalRNGStream.empty()){
        std::cout << "Traverse" << std::endl;

        hls::read_lock<IPage> page(pageIn);
        PageProperties p;
        memcpy(&p, &page[MAX_NODES_PER_PAGE], sizeof(PageProperties));
        
        //Init root node
        Node_hbm node;
        memcpy(&node, &page[0], sizeof(Node_hbm));

        unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
        float e[FEATURE_COUNT_TOTAL];
        bool endReached = false;

        hls::write_lock<IPage> b(pageOut);

        fill_output_loop: for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
            b[i] = page[i];
        }

        for(int n = 0; n < MAX_PAGE_DEPTH; n++){
        #pragma HLS PIPELINE OFF
        
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
                    memcpy(&b[node.idx], &node, sizeof(Node_hbm));

                    if(node.leaf){
                        endReached = true;
                    }else{
                        //Traverse
                        auto traverseChild = [&](auto &child){
                            if (!child.isPage) {
                                memcpy(&node, &page[child.nodeIdx], sizeof(Node_hbm));
                            } else {
                                p.nextPageIdx = child.pageIdx;
                                endReached = true;
                            }
                        };
                    
                        if(p.input.feature[node.feature] <= node.threshold){
                            traverseChild(node.leftChild);
                        }else{
                            traverseChild(node.rightChild);
                        }
                    }
                }
            }
        }
        memcpy(&b[MAX_NODES_PER_PAGE], &p, sizeof(PageProperties));
    }
}

void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut)
{
    #pragma HLS PIPELINE
    if(!pageIn.empty() && !splitterRNGStream.empty()){
        
        hls::read_lock<IPage> page(pageIn);
        PageProperties p;
        memcpy( &p, &page[MAX_NODES_PER_PAGE], sizeof(PageProperties));

        hls::write_lock<IPage> b(pageOut);
        for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
            b[i] = page[i];
        }

        if(p.split.split){
            std::cout << "Split" << std::endl;
            unit_interval upperBound, lowerBound;
            Node_hbm currentNode, parentNode, newNode, newSibbling;
            memcpy(&currentNode, &b[p.split.nodeIdx], sizeof(Node_hbm));
            memcpy(&parentNode, &b[p.split.parentIdx], sizeof(Node_hbm));
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
                memcpy(&child, &b[currentNode.leftChild.nodeIdx], sizeof(Node_hbm)); //CHANGE FOR PAGE (MAYBE POINTER/REFERENCE?)
                child.parentIdx = currentNode.idx;
                memcpy(&b[child.idx], &child, sizeof(Node_hbm));

                memcpy(&child, &b[currentNode.rightChild.nodeIdx], sizeof(Node_hbm));
                child.parentIdx = currentNode.idx;
                memcpy(&b[child.idx], &child, sizeof(Node_hbm));
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
            memcpy(&b[currentNode.idx], &currentNode, sizeof(Node_hbm));
            if(p.split.nodeIdx != p.split.parentIdx){
                memcpy(&b[parentNode.idx], &parentNode, sizeof(Node_hbm));
            }
            memcpy(&b[newNode.idx], &newNode, sizeof(Node_hbm));
            memcpy(&b[newSibbling.idx], &newSibbling, sizeof(Node_hbm));
        }
        memcpy(&b[MAX_NODES_PER_PAGE], &p, sizeof(PageProperties));
    }
}

void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool) //
{
    if(!pageIn.empty()){
        std::cout << "Save" << std::endl;
        hls::read_lock<IPage> page(pageIn);

        PageProperties p;
        memcpy( &p, &page[MAX_NODES_PER_PAGE], sizeof(PageProperties));

        for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
            memcpy(&pagePool[p.pageIdx][i], &page[i], sizeof(Node_hbm));
        }
        if(p.nextPageIdx != 0){
            feedbackStream.write(FetchRequest {.input = p.input, .pageIdx = p.nextPageIdx, .valid = false});
        }
    }
    

}