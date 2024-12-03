#include "train.hpp"
#include "hls_np_channel.h"

void train(
    hls::stream<FetchRequest> &fetchRequestStream,
    hls::stream<unit_interval> &traversalRNGStream,
    hls::stream<unit_interval> &splitterRNGStream,
    hls::stream<FetchRequest> &outputRequestStream,
    Page* pagePool
)
{
    #pragma HLS DATAFLOW
    #pragma HLS STABLE variable=pagePool
    #pragma HLS INTERFACE axis port=fetchRequestStream
    #pragma HLS INTERFACE axis port=traversalRNGStream
    #pragma HLS INTERFACE axis port=splitterRNGStream
    #pragma HLS INTERFACE axis port=outputRequestStream


    hls::split::round_robin<InternalPage, TREES_PER_BANK> fetchOutput("fetchOutputStream");

    hls::merge::round_robin<InternalPage, TREES_PER_BANK> traverseOutput("traverseOutputStream");

    hls::stream<InternalPage> splitterOut("SplitOutputStream");

    pre_fetcher(fetchRequestStream, fetchOutput.in, pagePool);
    // for(int i = 0; i < TREES_PER_BANK; i++){
    //     #pragma HLS UNROLL
    //     tree_traversal(fetchOutput.out[i], traversalRNGStream, traverseOutput.in[i]);
    // }
    tree_traversal(fetchOutput.out[0], traversalRNGStream, traverseOutput.in[0]);
    tree_traversal(fetchOutput.out[1], traversalRNGStream, traverseOutput.in[1]);
    tree_traversal(fetchOutput.out[2], traversalRNGStream, traverseOutput.in[2]);
    splitter(traverseOutput.out, splitterRNGStream, splitterOut);
    save(splitterOut, outputRequestStream, pagePool);

}

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<InternalPage> &pageOut, Page *pagePool)
{
    auto request = fetchRequestStream.read();
    InternalPage internal = {.feature=request.feature, .pageIdx=request.pageIdx};
    internal.page = pagePool[request.pageIdx];
    pageOut.write(internal);
}

void tree_traversal(hls::stream<InternalPage> &pageIn, hls::stream<unit_interval> &traversalRNGStream, hls::stream<InternalPage> &pageOut)
{
    InternalPage page = pageIn.read();

    Node_hbm node = page.page.nodes[page.page.rootNodeIdx];
    unit_interval e_l[FEATURE_COUNT_TOTAL], e_u[FEATURE_COUNT_TOTAL];
    
    bool endReached = false;
    for(int n = 0; n < MAX_PAGE_DEPTH; n++){ //Depth of the page
    #pragma HLS PIPELINE OFF
    
        if(!endReached){
            rate rate = 0;
            for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
                #pragma HLS PIPELINE
                e_l[i] = (node.lowerBound[i] > page.feature.data[i]) ? static_cast<unit_interval>(node.lowerBound[i] - page.feature.data[i]) : unit_interval(0);
                e_u[i] = (page.feature.data[i] > node.upperBound[i]) ? static_cast<unit_interval>(page.feature.data[i] - node.upperBound[i]) : unit_interval(0);
                rate += e_l[i] + e_u[i];
            }
            float E = -std::log(static_cast<float>(traversalRNGStream.read())) / static_cast<float>(rate); //TODO: change from log to hls::log
            if(node.parentSplitTime + E < node.splittime){
                page.split = true;
                page.splitIdx = node.idx;
                endReached = true;
            }else{
                for (int d = 0; d < FEATURE_COUNT_TOTAL; d++){ //TODO: not very efficient
                    if(e_l[d] != 0){
                        node.lowerBound[d] = page.feature.data[d];
                    }
                    if(e_u[d] !=0){
                        node.upperBound[d] = page.feature.data[d];
                    }
                }
                //Store changes to node
                page.page.nodes[node.idx] = node;
                
                //Traverse
                auto traverseChild = [&](auto &child){
                    if (!child.isPage) {
                        node = page.page.nodes[child.nodeIdx];
                    } else {
                        page.nextPageIdx = child.pageIdx;
                        endReached = true;
                    }
                };
               
                if(page.feature.data[node.feature] <= node.threshold){
                    traverseChild(node.leftChild);
                }else{
                    traverseChild(node.rightChild);
                }
            }
        }
    }
    pageOut.write(page);
}

void splitter(hls::stream<InternalPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream<InternalPage> &pageOut)
{
    InternalPage page = pageIn.read();
    if(page.split){
        splitterRNGStream.read();
    }
    pageOut.write(page);
}

void save(hls::stream<InternalPage> &pageIn, hls::stream<FetchRequest> &request, Page *pagePool)
{
    InternalPage page = pageIn.read();
    request.write(FetchRequest {.feature = page.feature, .pageIdx = page.nextPageIdx});
    pagePool[page.pageIdx] = page.page;
}