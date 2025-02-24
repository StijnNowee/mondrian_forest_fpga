#include "train.hpp"

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const int pageIdx, const Page *pagePool);

void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool)
{
    auto request = fetchRequestStream.read();
    burst_read_page(pageOut, request.input, request.treeID, request.pageIdx, pagePool);
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