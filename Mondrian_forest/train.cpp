#include "train.hpp"
#include <cstring>

void train(
    hls::stream<FetchRequest> &fetchRequestStream,
    hls::stream<unit_interval> &traversalRNGStream,
    hls::stream<unit_interval> &splitterRNGStream,
    hls::stream<FetchRequest> &outputRequestStream,
    Page* pagePool_read,
    Page* pagePool_write
)
{
    #pragma HLS DATAFLOW
    //#pragma HLS STABLE variable=pagePool
    #pragma HLS INTERFACE axis port=fetchRequestStream
    #pragma HLS INTERFACE axis port=traversalRNGStream
    #pragma HLS INTERFACE axis port=splitterRNGStream
    #pragma HLS INTERFACE axis port=outputRequestStream

//#pragma HLS DEPENDENCE dependent=false type=inter variable=pagePool

    //hls::split::round_robin<InternalPage, TREES_PER_BANK> fetchOutput("fetchOutputStream");
    hls::stream<PageChunk, MAX_NODES_PER_PAGE + 1> fetchOutput("fetchOutputStream");
    hls::stream<PageChunk, MAX_NODES_PER_PAGE + 1> traverseOutput("traverseOutputStream");
    //hls::merge::round_robin<InternalPage, TREES_PER_BANK> traverseOutput("traverseOutputStream");

    hls::stream<PageChunk, MAX_NODES_PER_PAGE + 1> splitterOut("SplitOutputStream");

    pre_fetcher(fetchRequestStream, fetchOutput, pagePool_read);
    // for(int i = 0; i < TREES_PER_BANK; i++){
    //     #pragma HLS UNROLL
    //     tree_traversal(fetchOutput.out[i], traversalRNGStream, traverseOutput.in[i]);
    // }
    tree_traversal(fetchOutput, traversalRNGStream, traverseOutput);
    //tree_traversal(fetchOutput.out[1], traversalRNGStream, traverseOutput.in[1]);
    //tree_traversal(fetchOutput.out[2], traversalRNGStream, traverseOutput.in[2]);
    splitter(traverseOutput, splitterRNGStream, splitterOut);
    save(splitterOut, outputRequestStream, pagePool_write);
}

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<PageChunk> &pageOut, const Page *pagePool)
{
    auto request = fetchRequestStream.read();
    //InternalPage internal = {.feature=request.feature, .pageIdx=request.pageIdx};
    //Page page;
    //std::memcpy(&page, &pagePool[request.pageIdx], sizeof(Page));
    Page page = pagePool[request.pageIdx];
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
         pageOut.write(PageChunk(page.nodes[i]));
    }
    PageProperties p = {.feature = request.feature, .pageIdx=request.pageIdx};
    pageOut.write(PageChunk(p));
   
}

void tree_traversal(hls::stream<PageChunk> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream<PageChunk> &pageOut)
{
    //InternalPage page = pageIn.read();

    Page page;
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        page.nodes[i] = pageIn.read().node;
    }
    PageProperties p = pageIn.read().p;

    Node_hbm node = page.nodes[p.rootNodeIdx];
    unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
    bool endReached = false;
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
                page.nodes[node.idx] = node;
                
                //Traverse
                auto traverseChild = [&](auto &child){
                    if (!child.isPage) {
                        node = page.nodes[child.nodeIdx];
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
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        pageOut.write(PageChunk(page.nodes[i]));
    }
    pageOut.write(PageChunk(p));
}

void splitter(hls::stream<PageChunk> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream<PageChunk> &pageOut)
{
    //InternalPage page = pageIn.read();
    Page page;
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        page.nodes[i] = pageIn.read().node;
    }
    PageProperties p = pageIn.read().p;

    if(p.split){
        splitterRNGStream.read();
    }

    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        pageOut.write(PageChunk(page.nodes[i]));
    }
    pageOut.write(PageChunk(p));
}

void save(hls::stream<PageChunk> &pageIn, hls::stream<FetchRequest> &request, Page *pagePool)
{
    Page page;
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        page.nodes[i] = pageIn.read().node;
    }
    PageProperties p = pageIn.read().p;

    request.write(FetchRequest {.feature = p.feature, .pageIdx = p.nextPageIdx});
    //pagePool[p.pageIdx] = page;
    for (size_t i = 0; i < MAX_NODES_PER_PAGE; i++) {
        #pragma HLS PIPELINE
        pagePool[p.pageIdx].nodes[i] = page.nodes[i];
    }
    //std::memcpy(&pagePool[p.pageIdx], &page, sizeof(Page));
}