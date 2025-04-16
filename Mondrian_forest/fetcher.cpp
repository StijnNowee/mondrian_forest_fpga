#include "processing_unit.hpp"
#include "converters.hpp"
template<typename T>
void burst_read_page(IPage pageOut, FetchRequest &request, const Page *pageBank);

template <int TRAVERSAL_BLOCKS, typename T, typename P>
void fetcher(hls::stream<T> &fetchRequestStream, hls::stream_of_blocks<IPage> pageOutS[TRAVERSAL_BLOCKS], const Page *pageBank, const int &blockIdx)
{
    
    int traverseBlockId = TRAIN_TRAVERSAL_BLOCKS;
    for(int b = 0; b < TRAIN_TRAVERSAL_BLOCKS; b++){
        #pragma HLS PIPELINE II=1
        int idx =  (b + blockIdx) % 3;
        if(!pageOutS[idx].full()){
            traverseBlockId = idx;
        }
    }
    if(!fetchRequestStream.empty() && traverseBlockId != TRAIN_TRAVERSAL_BLOCKS){
        T request = fetchRequestStream.read();
        hls::write_lock<IPage> pageOut(pageOutS[traverseBlockId]);
        burst_read_page<T, P>(pageOut, request, pageBank);
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
    const Page *pageBank,
    const int &blockIdx
);

template void fetcher<INF_TRAVERSAL_BLOCKS, IFetchRequest, IPageProperties>(
    hls::stream<IFetchRequest> &fetchRequestStream,
    hls::stream_of_blocks<IPage> pageOutS[INF_TRAVERSAL_BLOCKS], 
    const Page *pageBank,
    const int &blockIdx
);
