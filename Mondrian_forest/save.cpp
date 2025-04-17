#include "train.hpp"
#include "converters.hpp"

void save_to_memory(const Page &page, const int &globalPageIdx, Page *pageBank);

void save(hls::stream_of_blocks<Page> pageIn[2], hls::stream<Feedback> &feedbackStream, hls::stream<PageProperties> &pagePropertyStream, Page *pageBank) //
{
    bool extraPage = false;
    if(!pageIn[0].empty()){
        auto p = pagePropertyStream.read();
        int globalPageIdx[2];
        globalPageIdx[0] = p.treeID * MAX_PAGES_PER_TREE + p.pageIdx;
        
        globalPageIdx[1] = (p.extraPage) ? p.treeID * MAX_PAGES_PER_TREE + p.freePageIdx : 0;
        {
        hls::read_lock<Page> page(pageIn[0]);
        save_to_memory(page, globalPageIdx[0], pageBank);
        }
        if(p.extraPage){
            hls::read_lock<Page> page(pageIn[1]);
            save_to_memory(page, globalPageIdx[1], pageBank);
        }
       
        Feedback feedback(p, p.extraPage);
        feedbackStream.write(feedback);    
    }
}

void save_to_memory(const Page &page, const int &globalPageIdx, Page *pageBank){
    #pragma HLS inline
    
    write_to_memory: for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        #pragma HLS unroll off
        #pragma HLS PIPELINE II=1
        pageBank[globalPageIdx][n] = page[n];
    }
}

    // if(!save1.empty()){
    //     bool extraPage = false;
    //     if(!save2.empty()){
            
    //         hls::read_lock<IPage> pageIn2(save2);
    //         const PageProperties p2 = rawToProperties<PageProperties>(pageIn2[MAX_NODES_PER_PAGE]);
    //         if(p2.shouldSave){
    //             save_to_memory(pageIn2, p2.treeID, p2.freePageIdx, pageBank);
    //             extraPage = true;
    //         }
    //     }
    //     hls::read_lock<IPage> pageIn1(save1);
    //     const PageProperties p1 = rawToProperties<PageProperties>(pageIn1[MAX_NODES_PER_PAGE]);
    //     save_to_memory(pageIn1, p1.treeID, p1.pageIdx, pageBank);

    //     Feedback feedback(p1, extraPage);
    //     feedbackStream.write(feedback);
    // }