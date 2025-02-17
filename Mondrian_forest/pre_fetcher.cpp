#include "train.hpp"

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const int pageIdx, const Page *pagePool);

//TODO overhaul, use fetchRequests to handle this
void pre_fetcher(hls::stream<input_vector> &splitFeatureStream, hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool)
{
    //Initialise status
    #pragma HLS stable variable=pagePool
    static TreeStatus status[TREES_PER_BANK] = {IDLE};
    static int scheduled[TREES_PER_BANK] = {2};
    int nrScheduled = 0;
    // for (int i = 0; i < TREES_PER_BANK; i++) {
    //     status[i] = IDLE;
    //     scheduled[i] = TREES_PER_BANK + 1;
    // }
        // Prioritize processing feedback requests from the feedback stream.
    if(!feedbackStream.empty()){
        FetchRequest request = feedbackStream.read();
        if(request.needNewPage){
            //Tree needs new page
            burst_read_page(pageOut, request.input, request.treeID, request.pageIdx, pagePool);
        }else if (request.done){
            //Tree finished processing
            status[request.treeID] = IDLE;
        }
    } else{
        // If no feedback, check for new input vectors for idle trees.
        check_idle_trees: for(int t = 0; t < TREES_PER_BANK; t++){
            if(status[t] == IDLE && !splitFeatureStream.empty()){
                scheduled[nrScheduled++] = t;
                #if(defined __SYNTHESIS__)
                    status[t] = PROCESSING;
                #endif
            }
        }
        if(nrScheduled > 0){
            scheduled_loop: for(int st = 0; st < nrScheduled; ++st){
                #pragma HLS PIPELINE II=MAX_NODES_PER_PAGE
                auto input = splitFeatureStream.read();
                //Fetch the first page for the tree
                burst_read_page(pageOut, input, scheduled[st], 0, pagePool);
            }
            nrScheduled = 0;
        }
    }
}


void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const int pageIdx, const Page *pagePool)
{
    //#pragma HLS stable variable=pagePool
    #pragma HLS inline off
    //Read from memory
    const int globalPageIdx = treeID * MAX_PAGES_PER_TREE + pageIdx;
    hls::write_lock<IPage> out(pageOut);
    for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        out[i] = pagePool[globalPageIdx][i];
    }

    PageProperties p(feature, pageIdx, treeID);

    out[MAX_NODES_PER_PAGE] = convertProperties(p);
}