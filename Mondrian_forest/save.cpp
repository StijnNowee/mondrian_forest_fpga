#include "train.hpp"
#include "converters.hpp"

void save_to_memory(const IPage page, const int treeID, const int pageIdx, Page *pageBank);

void save(hls::stream_of_blocks<IPage> &save1, hls::stream_of_blocks<IPage> &save2, hls::stream<Feedback> &feedbackStream, Page *pageBank) //
{
    if(!save1.empty()){
        bool extraPage = false;
        if(!save2.empty()){
            
            hls::read_lock<IPage> pageIn2(save2);
            const PageProperties p2 = rawToProperties<PageProperties>(pageIn2[MAX_NODES_PER_PAGE]);
            if(p2.shouldSave){
                save_to_memory(pageIn2, p2.treeID, p2.freePageIdx, pageBank);
                extraPage = true;
            }
        }
        hls::read_lock<IPage> pageIn1(save1);
        const PageProperties p1 = rawToProperties<PageProperties>(pageIn1[MAX_NODES_PER_PAGE]);
        save_to_memory(pageIn1, p1.treeID, p1.pageIdx, pageBank);

        Feedback feedback(p1, extraPage);
        feedbackStream.write(feedback);
    }
}

void save_to_memory(const IPage page, const int treeID, const int pageIdx, Page *pageBank){
    const int globalPageIdx = treeID * MAX_PAGES_PER_TREE + pageIdx;
    write_to_memory: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        #pragma HLS PIPELINE II=1
        pageBank[globalPageIdx][i] = page[i];
    }
}

