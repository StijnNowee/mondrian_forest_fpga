#include "train.hpp"
#include "converters.hpp"
#include <etc/autopilot_ssdm_op.h>

void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream, bool rootPage);
void processPage(const IPage page, Page *pagePool, hls::stream<FetchRequest> &feedbackStream);

void save(const IPage pageIn1, const IPage pageIn2, hls::stream<FetchRequest> &feedbackStream, Page *pagePool) //
{
    std::cout << "Save baby save" << std::endl;
    processPage(pageIn1, pagePool, feedbackStream);
    processPage(pageIn2, pagePool, feedbackStream);
    
}

void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream, bool rootPage)
{
        //Race condition blocker
        // if(rootPage && !request.needNewPage){
        //     ap_wait_n(150);
        // }
        feedbackStream.write(request);
}

void processPage(const IPage page,  Page *pagePool, hls::stream<FetchRequest> &feedbackStream){
    PageProperties p = rawToProperties(page[MAX_NODES_PER_PAGE]);
    if(p.shouldSave){
        int globalPageIdx = p.treeID * MAX_PAGES_PER_TREE + p.pageIdx;
        
        write_to_memory: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            #pragma HLS PIPELINE II=1
            pagePool[globalPageIdx][i] = page[i];
        }
        
        //Create new request
        auto request = FetchRequest {.input = p.input, .pageIdx = p.nextPageIdx, .treeID = p.treeID,  .extraPage = p.extraPage, .needNewPage = p.needNewPage};
        
        //Race condition blocker
        sendFeedback(request, feedbackStream, p.pageIdx == 0);
    }
    // if(!p.extraPage && !p.needNewPage){
    //     // iter++;
    // }
}