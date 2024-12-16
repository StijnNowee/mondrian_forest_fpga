#include "train.hpp"
#include <cstring>
#include <cwchar>

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool)
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
        PageProperties p = {.feature = request.feature, .pageIdx=request.pageIdx};
        memcpy(&b[MAX_NODES_PER_PAGE], &p, sizeof(PageProperties));
    }
}

void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut)
{
    #pragma HLS PIPELINE
    std::cout << "Traverse" << std::endl;
    if(!pageIn.empty() && !traversalRNGStream.empty()){
        hls::read_lock<IPage> page(pageIn);
        PageProperties p;
        std::cout << "Read pageProperties" << std::endl;
        memcpy(&p, &page[MAX_NODES_PER_PAGE], sizeof(PageProperties));
        std::cout << "Page idx: " << p.pageIdx << std::endl;
        
        Node_hbm node;
        std::cout << "Read node: " << p.rootNodeIdx <<  std::endl;
        memcpy(&node, &page[p.rootNodeIdx], sizeof(Node_hbm));
        std::cout << "my idx: " << node.idx << std::endl;
        unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
        bool endReached = false;

        hls::write_lock<IPage> b(pageOut);

        fill_output_loop: for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
            b[i] = page[i];
        }
        std::cout << "Run loop" << std::endl;

        for(int n = 0; n < MAX_PAGE_DEPTH; n++){
        #pragma HLS PIPELINE OFF
        
            if(!endReached){
                rate rate = 0;
                for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
                    #pragma HLS PIPELINE
                    e_l[i] = (node.lowerBound[i] > p.feature.data[i]) ? static_cast<unit_interval>(node.lowerBound[i] - p.feature.data[i]) : unit_interval(0);
                    e_u[i] = (p.feature.data[i] > node.upperBound[i]) ? static_cast<unit_interval>(p.feature.data[i] - node.upperBound[i]) : unit_interval(0);
                    rate += e_l[i] + e_u[i];
                }
                std::cout << "After feature loop" << std::endl;
                float E = -std::log(static_cast<float>(traversalRNGStream.read())) / static_cast<float>(rate); //TODO: change from log to hls::log
                if(node.parentSplitTime + E < node.splittime){
                    p.split = true;
                    p.splitIdx = node.idx;
                    endReached = true;
                }else{
                    std::cout << "Traverse further" << std::endl;
                    for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: not very efficient
                        if(e_l[d] != 0){
                            node.lowerBound[d] = p.feature.data[d];
                        }
                        if(e_u[d] !=0){
                            node.upperBound[d] = p.feature.data[d];
                        }
                    }
                    std::cout << "Store: " << node.idx << std::endl;
                    //Store changes to node
                    memcpy(&b[node.idx], &node, sizeof(Node_hbm));
                    std::cout << "Done storing" << std::endl;
                    //Traverse
                    auto traverseChild = [&](auto &child){
                        if (!child.isPage) {
                            memcpy(&node, &page[child.nodeIdx], sizeof(Node_hbm));
                        } else {
                            p.nextPageIdx = child.pageIdx;
                            endReached = true;
                        }
                    };
                
                    if(p.feature.data[node.feature] <= node.threshold){
                        traverseChild(node.leftChild);
                    }else{
                        traverseChild(node.rightChild);
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
        std::cout << "Split" << std::endl;
        hls::read_lock<IPage> page(pageIn);
        PageProperties p;
        memcpy( &p, &page[MAX_NODES_PER_PAGE], sizeof(PageProperties));

        //if(p.split){
            splitterRNGStream.read();
        //}

        hls::write_lock<IPage> b(pageOut);
        for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
            b[i] = page[i];
        }
        memcpy(&b[MAX_NODES_PER_PAGE], &p, sizeof(PageProperties));
    }
}

void save(hls::stream_of_blocks<IPage> &pageIn, Page *pagePool) //hls::stream<FetchRequest> &feedbackStream,
{
    #pragma HLS PIPELINE
    if(!pageIn.empty()){
        std::cout << "Save" << std::endl;
        hls::read_lock<IPage> page(pageIn);

        PageProperties p;
        memcpy( &p, &page[MAX_NODES_PER_PAGE], sizeof(PageProperties));

        for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
            memcpy(&pagePool[p.pageIdx], &page[i], sizeof(Node_hbm));
        }
        //feedbackStream.write(FetchRequest {.feature = p.feature, .pageIdx = p.nextPageIdx});
    }
    

}