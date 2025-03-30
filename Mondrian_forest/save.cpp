#include "train.hpp"
#include "converters.hpp"
#include <etc/autopilot_ssdm_op.h>

void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream, bool rootPage);

void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool, const int size) //
{
    for(int iter = 0; iter < size*TREES_PER_BANK;){
        if(!pageIn.empty()){
            hls::read_lock<IPage> localPage(pageIn);
            PageProperties p = rawToProperties(localPage[MAX_NODES_PER_PAGE]);
            
            int globalPageIdx = p.treeID * MAX_PAGES_PER_TREE + p.pageIdx;
            
            write_to_memory: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
                #pragma HLS PIPELINE II=1
                pagePool[globalPageIdx][i] = localPage[i];
            }
            
            //Create new request
            auto request = FetchRequest {.input = p.input, .pageIdx = p.nextPageIdx, .treeID = p.treeID,  .extraPage = p.extraPage, .needNewPage = p.needNewPage};
            
            //Race condition blocker
            sendFeedback(request, feedbackStream, p.pageIdx == 0);
            if(!p.extraPage && !p.needNewPage){
                iter++;
            }
        }
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