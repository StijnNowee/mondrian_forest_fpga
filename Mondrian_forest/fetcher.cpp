#include "processing_unit.hpp"
#include "converters.hpp"
template<typename T>
void burst_read_page(IPage pageOut, FetchRequest &request, const Page *pageBank);

template <int TRAVERSAL_BLOCKS, typename T, typename P>
void fetcher(hls::stream<T> &fetchRequestStream, hls::stream_of_blocks<IPage> pageOutS[TRAVERSAL_BLOCKS], const Page *pageBank)
{
    int availableBlocks = 0;
    T requestList[TRAVERSAL_BLOCKS];
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
        burst_read_page<T, P>(pageOut, requestList[b], pageBank);
    }
}

template<typename T, typename P>
void burst_read_page(IPage pageOut, T &request, const Page *pageBank)
{
    #pragma HLS inline
    const int globalPageIdx = request.treeID * MAX_PAGES_PER_TREE + request.pageIdx;
    for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        //Pipeline II=1 is causing congestion issues each node 1024 wide
        #pragma HLS PIPELINE II=2
        pageOut[i] = pageBank[globalPageIdx][i];
    }

    P p(request);
    pageOut[MAX_NODES_PER_PAGE] = propertiesToRaw<P>(p);
}

template void fetcher<TRAIN_TRAVERSAL_BLOCKS, FetchRequest, PageProperties>(
    hls::stream<FetchRequest> &fetchRequestStream,
    hls::stream_of_blocks<IPage> pageOutS[TRAIN_TRAVERSAL_BLOCKS], 
    const Page *pageBank
);

template void fetcher<INF_TRAVERSAL_BLOCKS, IFetchRequest, IPageProperties>(
    hls::stream<IFetchRequest> &fetchRequestStream,
    hls::stream_of_blocks<IPage> pageOutS[INF_TRAVERSAL_BLOCKS], 
    const Page *pageBank
);
