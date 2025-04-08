#include "train.hpp"
#include "converters.hpp"

void save_to_memory(const IPage page, const int treeID, const int pageIdx, Page *pagePool);

void save(hls::stream_of_blocks<IPage> &save1, hls::stream_of_blocks<IPage> &save2, hls::stream<Feedback> &feedbackStream, Page *pagePool) //
{
    if(!save1.empty()){
        bool extraPage = false;
        if(!save2.empty()){
            
            hls::read_lock<IPage> pageIn2(save2);
            const PageProperties p2 = rawToProperties(pageIn2[MAX_NODES_PER_PAGE]);
            if(p2.shouldSave){
                save_to_memory(pageIn2, p2.treeID, p2.freePageIdx, pagePool);
                extraPage = true;
            }
        }
        hls::read_lock<IPage> pageIn1(save1);
        const PageProperties p1 = rawToProperties(pageIn1[MAX_NODES_PER_PAGE]);
        save_to_memory(pageIn1, p1.treeID, p1.pageIdx, pagePool);

        Feedback feedback(p1, extraPage);
        feedbackStream.write(feedback);
    }
}

void save_to_memory(const IPage page, const int treeID, const int pageIdx, Page *pagePool){
    const int globalPageIdx = treeID * MAX_PAGES_PER_TREE + pageIdx;
    write_to_memory: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        #pragma HLS PIPELINE II=1
        pagePool[globalPageIdx][i] = page[i];
    }
}

