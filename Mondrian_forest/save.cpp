#include "train.hpp"
#include <etc/autopilot_ssdm_op.h>

void sendFeedback(FetchRequest request, hls::stream<FetchRequest> &feedbackStream, bool rootPage);

void save(hls::stream_of_blocks<IPage> &pageIn, hls::stream<FetchRequest> &feedbackStream, Page *pagePool, const int loopCount, hls::stream<bool> &treeDoneStream) //
{
    main_loop: for(int iter = 0; iter < loopCount;){
        #pragma HLS PIPELINE
        if(!pageIn.empty()){
            hls::read_lock<IPage> in(pageIn);

            auto p = convertProperties(in[MAX_NODES_PER_PAGE]);
            
            int globalPageIdx = p.treeID * MAX_PAGES_PER_TREE + p.pageIdx;
            write_to_memory: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
                //#pragma HLS PIPELINE II=5
                pagePool[globalPageIdx][i] = in[i];
            }
            //Create new request
            auto request = FetchRequest {.input = p.input, .pageIdx = p.nextPageIdx, .treeID = p.treeID,  .done = !p.extraPage, .needNewPage = p.needNewPage};
            
            //Race condition blocker
            sendFeedback(request, feedbackStream, p.pageIdx == 0);
            #if(not defined __SYNTHESIS__)
                if(!p.extraPage){
                    iter++;
                }
            #endif
        }

        #if(defined __SYNTHESIS__)
            if(!treeDoneStream.empty()){
                treeDoneStream.read();
                iter++;
            }
        #endif
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