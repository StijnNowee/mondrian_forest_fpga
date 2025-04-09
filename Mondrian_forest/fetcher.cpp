#include "train.hpp"
#include "converters.hpp"
#include <iostream>

void burst_read_page(IPage pageOut, FetchRequest &request, const PageBank &pageBank);

template <int TRAVERSAL_BLOCKS>
void fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> pageOutS[TRAVERSAL_BLOCKS], const PageBank &pageBank)
{
    int availableBlocks = 0;
    FetchRequest requestList[TRAVERSAL_BLOCKS];
    int availableBlockIds[TRAVERSAL_BLOCKS];
    for(int b = 0; b < TRAVERSAL_BLOCKS; b++){
        if(!pageOutS[b].full() && !fetchRequestStream.empty()){
            requestList[availableBlocks] = fetchRequestStream.read();
            availableBlockIds[availableBlocks++] = b;
        }
    }
    for(int b = 0; b < availableBlocks; b++){
        #pragma HLS LOOP_TRIPCOUNT max=TRAVERSAL_BLOCKS min=0
        hls::write_lock<IPage> pageOut(pageOutS[availableBlockIds[b]]);
        burst_read_page(pageOut, requestList[b], pageBank);
    }
}

void burst_read_page(IPage pageOut, FetchRequest &request, const PageBank &pageBank)
{
    #pragma HLS inline
    const int globalPageIdx = request.treeID * MAX_PAGES_PER_TREE + request.pageIdx;
    for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        #pragma HLS PIPELINE II=1
        pageOut[i] = pageBank[globalPageIdx][i];
    }

    PageProperties p(request);
    pageOut[MAX_NODES_PER_PAGE] = propertiesToRaw(p);
}