#include "train.hpp"
#include <cstring>
#include <cwchar>


void train(
    hls::stream<FetchRequest> &fetchRequestStream,
    hls::stream<unit_interval> &traversalRNGStream,
    hls::stream<unit_interval> &splitterRNGStream,
    hls::stream<FetchRequest> &outputRequestStream,
    const Page* pagePool_read,
    Page* pagePool_write
)
{
    #pragma HLS DATAFLOW
    #pragma HLS INTERFACE axis port=fetchRequestStream
    #pragma HLS INTERFACE axis port=traversalRNGStream
    #pragma HLS INTERFACE axis port=splitterRNGStream
    #pragma HLS INTERFACE axis port=outputRequestStream

    hls::stream_of_blocks<IPage> fetchOutput;
    hls::stream_of_blocks<IPage> traverseOutput;
    hls::stream_of_blocks<IPage> splitterOut;

    pre_fetcher(fetchRequestStream, fetchOutput, pagePool_read);
    tree_traversal(fetchOutput, traversalRNGStream, traverseOutput);
    splitter(traverseOutput, splitterRNGStream, splitterOut);
    save(splitterOut, outputRequestStream);
}

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool)
{
    auto request = fetchRequestStream.read();
    hls::write_lock<IPage> b(pageOut);
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        #pragma HLS PIPELINE
        memcpy(&b[i], &pagePool[request.pageIdx][i], sizeof(Node_hbm));
    }
    PageProperties p = {.feature = request.feature, .pageIdx=request.pageIdx};
    memcpy(&b[MAX_NODES_PER_PAGE], &p, sizeof(PageProperties));
    
   
}

void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut)
{
    hls::read_lock<IPage> page(pageIn);
    PageProperties p;
    memcpy(&p, &page[MAX_NODES_PER_PAGE], sizeof(PageProperties));
    
    Node_hbm node;
    memcpy(&node, &page[p.rootNodeIdx], sizeof(Node_hbm));
    unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
    bool endReached = false;

    hls::write_lock<IPage> b(pageOut);
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        b[i] = page[i];
    }

    for(int n = 0; n < MAX_PAGE_DEPTH; n++){ //Depth of the page
    #pragma HLS PIPELINE OFF
    
        if(!endReached){
            rate rate = 0;
            for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
                #pragma HLS PIPELINE
                e_l[i] = (node.lowerBound[i] > p.feature.data[i]) ? static_cast<unit_interval>(node.lowerBound[i] - p.feature.data[i]) : unit_interval(0);
                e_u[i] = (p.feature.data[i] > node.upperBound[i]) ? static_cast<unit_interval>(p.feature.data[i] - node.upperBound[i]) : unit_interval(0);
                rate += e_l[i] + e_u[i];
            }
            float E = -std::log(static_cast<float>(traversalRNGStream.read())) / static_cast<float>(rate); //TODO: change from log to hls::log
            if(node.parentSplitTime + E < node.splittime){
                p.split = true;
                p.splitIdx = node.idx;
                endReached = true;
            }else{
                for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: not very efficient
                    if(e_l[d] != 0){
                        node.lowerBound[d] = p.feature.data[d];
                    }
                    if(e_u[d] !=0){
                        node.upperBound[d] = p.feature.data[d];
                    }
                }
                //Store changes to node
                memcpy(&b[node.idx], &node, sizeof(Node_hbm));
                
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

void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut)
{
    hls::read_lock<IPage> page(pageIn);
    PageProperties p;
    memcpy( &p, &page[MAX_NODES_PER_PAGE], sizeof(PageProperties));

    if(p.split){
        splitterRNGStream.read();
    }

    hls::write_lock<IPage> b(pageOut);
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        b[i] = page[i];
    }
    memcpy(&b[MAX_NODES_PER_PAGE], &p, sizeof(PageProperties));
}

void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &request)
{
    
    hls::read_lock<IPage> page(pageIn);

    PageProperties p;
    memcpy( &p, &page[MAX_NODES_PER_PAGE], sizeof(PageProperties));

    request.write(FetchRequest {.feature = p.feature, .pageIdx = p.nextPageIdx});

}