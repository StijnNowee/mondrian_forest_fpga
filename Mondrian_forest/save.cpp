#include "train.hpp"
#include <etc/autopilot_ssdm_op.h>

void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream, bool rootPage);

void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, hls::stream<bool> &controlOutputStream, Page *pagePool, const int size) //
{
    
    IPage localPage;
    for(int iter = 0; iter < size;){
    //#pragma HLS PIPELINE
    if(!pageIn.empty()){
        std::cout << "SAVING\n";
        PageProperties p;
        read_page(localPage, p, pageIn);
        //outputStream.write(in[MAX_NODES_PER_PAGE]);
        std::cout << "read page\n";
        
        int globalPageIdx = p.treeID * MAX_PAGES_PER_TREE + p.pageIdx;
        write_to_memory: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            //#pragma HLS PIPELINE II=5
            std::cout << "write node: " << i << "\n";
            pagePool[globalPageIdx][i] = localPage[i];
            //if(!p.extraPage && !p.needNewPage){
            //outputStream.write(in[i]);
            //}
        }
        
        //Create new request
        std::cout << "Create new request\n";
        auto request = FetchRequest {.input = p.input, .pageIdx = p.nextPageIdx, .treeID = p.treeID,  .done = !p.extraPage, .needNewPage = p.needNewPage};
        
        //Race condition blocker
        std::cout << "Send feedback\n";
        sendFeedback(request, feedbackStream, p.pageIdx == 0);
        if(!p.extraPage && !p.needNewPage){
            iter++;
            // std::cout << "Write to controlStream" << std::endl;
            // controlOutputStream.write(true);
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