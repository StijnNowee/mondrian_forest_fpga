#include "train.hpp"
#include <cstring>


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
    //#pragma HLS STABLE variable=pagePool
    #pragma HLS INTERFACE axis port=fetchRequestStream
    #pragma HLS INTERFACE axis port=traversalRNGStream
    #pragma HLS INTERFACE axis port=splitterRNGStream
    #pragma HLS INTERFACE axis port=outputRequestStream

    hls::stream_of_blocks<IPage> fetchOutput;
    hls::stream_of_blocks<IPage> traverseOutput;
    hls::stream_of_blocks<IPage> splitterOut;


    //hls::split::round_robin<InternalPage, TREES_PER_BANK> fetchOutput("fetchOutputStream");
    //hls::stream<PageChunk, (MAX_NODES_PER_PAGE + 1) * 10> fetchOutput("fetchOutputStream");
    //hls::stream<PageChunk, (MAX_NODES_PER_PAGE + 1) * 10> traverseOutput("traverseOutputStream");
    //hls::merge::round_robin<InternalPage, TREES_PER_BANK> traverseOutput("traverseOutputStream");

    //hls::stream<PageChunk, (MAX_NODES_PER_PAGE + 1) * 10> splitterOut("SplitOutputStream");

    pre_fetcher(fetchRequestStream, fetchOutput, pagePool_read);
    // for(int i = 0; i < TREES_PER_BANK; i++){
    //     #pragma HLS UNROLL
    //     tree_traversal(fetchOutput.out[i], traversalRNGStream, traverseOutput.in[i]);
    // }
    tree_traversal(fetchOutput, traversalRNGStream, traverseOutput);
    //tree_traversal(fetchOutput.out[1], traversalRNGStream, traverseOutput.in[1]);
    //tree_traversal(fetchOutput.out[2], traversalRNGStream, traverseOutput.in[2]);
    splitter(traverseOutput, splitterRNGStream, splitterOut);
    save(splitterOut, outputRequestStream);
}

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool)
{
    std::cout << "preFetcher" << std::endl;
    auto request = fetchRequestStream.read();
    hls::write_lock<IPage> b(pageOut);
        PageChunk chunk;
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        #pragma HLS PIPELINE
        chunk.node = pagePool[request.pageIdx][i];
        b[i] = chunk;
         //pageOut.write();
    }
    PageProperties p = {.feature = request.feature, .pageIdx=request.pageIdx};
    chunk.p = p;
    
    b[MAX_NODES_PER_PAGE] = chunk;
   
   
}

void tree_traversal(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream_of_blocks<IPage> &pageOut)
{
   std::cout << "Tree_traversal" << std::endl;

    // Page page;
    hls::read_lock<IPage> page(pageIn);
    // for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
    //     page[i] = pageIn.read().node;
    // }
    PageProperties p = page[MAX_NODES_PER_PAGE].p;
    
    Node_hbm node = page[p.rootNodeIdx].node;
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
                PageChunk chunk;
                chunk.node = node;
                page[node.idx] = chunk;
                
                //Traverse
                auto traverseChild = [&](auto &child){
                    if (!child.isPage) {
                        node = page[child.nodeIdx].node;
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
    hls::write_lock<IPage> b(pageOut);
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        b[i] = page[i];
    }
    PageChunk chunk;
    chunk.p = p;
    b[MAX_NODES_PER_PAGE] = chunk;
    // pageOut.write(PageChunk(p));
}

void splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut)
{
    std::cout << "Splitter" << std::endl;
    hls::read_lock<IPage> page(pageIn);
    // for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
    //     page[i] = pageIn.read().node;
    // }
    PageProperties p = page[MAX_NODES_PER_PAGE].p;

    if(p.split){
        splitterRNGStream.read();
    }

    hls::write_lock<IPage> b(pageOut);
    for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
        b[i] = page[i];
    }
        PageChunk chunk;
    chunk.p = p;
    b[MAX_NODES_PER_PAGE] = chunk;
}

void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &request)
{
    std::cout << "Save" << std::endl;
    hls::read_lock<IPage> page(pageIn);
    // for(size_t i = 0; i < MAX_NODES_PER_PAGE; i++){
    //     page[i] = pageIn.read().node;
    // }
    auto p = page[MAX_NODES_PER_PAGE].p;

    // for (size_t i = 0; i < MAX_NODES_PER_PAGE; i++) {
    //     #pragma HLS PIPELINE
    //     pagePool[p.pageIdx][i] = page[i];
    // }

    request.write(FetchRequest {.feature = p.feature, .pageIdx = p.nextPageIdx});

}