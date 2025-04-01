#include "train.hpp"
#include "converters.hpp"
#include <etc/autopilot_ssdm_op.h>

void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream);
void processPage(const IPage page, Page *pagePool, hls::stream<FetchRequest> &feedbackStream);
void save_to_memory(const IPage page, PageProperties &p, Page *pagePool);

void save(hls::stream_of_blocks<IPage> &save1, hls::stream_of_blocks<IPage> &save2, hls::stream<FetchRequest> &feedbackStream, Page *pagePool) //
{
    if(!save1.empty()){
        bool extraPage = false;
        hls::read_lock<IPage> pageIn1(save1);
        hls::read_lock<IPage> pageIn2(save2);
        PageProperties p1 = rawToProperties(pageIn1[MAX_NODES_PER_PAGE]);
        PageProperties p2 = rawToProperties(pageIn2[MAX_NODES_PER_PAGE]);
        
        save_to_memory(pageIn1, p1, pagePool);

        if(p2.shouldSave){
            save_to_memory(pageIn2, p2, pagePool);
            extraPage = true;
        }
        auto request = FetchRequest {.input = p1.input, .pageIdx = p1.nextPageIdx, .treeID = p1.treeID,  .extraPage = extraPage, .needNewPage = p1.needNewPage};
        feedbackStream.write(request);
    }
    
}

void save_to_memory(const IPage page, PageProperties &p, Page *pagePool){
    const int globalPageIdx = p.treeID * MAX_PAGES_PER_TREE + p.pageIdx;
    write_to_memory: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        #pragma HLS PIPELINE II=1
        pagePool[globalPageIdx][i] = page[i];
    }
}

void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream)
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
        //sendFeedback(request, feedbackStream, p.pageIdx == 0);
    }
    // if(!p.extraPage && !p.needNewPage){
    //     // iter++;
    // }
}