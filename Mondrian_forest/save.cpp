#include "train.hpp"
#include <etc/autopilot_ssdm_op.h>

void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream, bool rootPage);

void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, hls::stream<node_t> &outputStream, Page *pagePool) //
{
    #pragma HLS PIPELINE
    hls::read_lock<IPage> in(pageIn);

    PageProperties p = convertProperties(in[MAX_NODES_PER_PAGE]);
    //outputStream.write(in[MAX_NODES_PER_PAGE]);
    
    int globalPageIdx = p.treeID * MAX_PAGES_PER_TREE + p.pageIdx;
    write_to_memory: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        //#pragma HLS PIPELINE II=5
        pagePool[globalPageIdx][i] = in[i];
        //outputStream.write(in[i]);
    }
    //Create new request
    auto request = FetchRequest {.input = p.input, .pageIdx = p.nextPageIdx, .treeID = p.treeID,  .done = !p.extraPage, .needNewPage = p.needNewPage};
    
    //Race condition blocker
    sendFeedback(request, feedbackStream, p.pageIdx == 0);
    if(!p.extraPage && !p.needNewPage){
        outputStream.write(1);
    }
}

void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream, bool rootPage)
{
        //Race condition blocker
        if(rootPage && !request.needNewPage){
            ap_wait_n(150);
        }
        feedbackStream.write(request);
}