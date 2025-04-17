#include "train.hpp"
#include "converters.hpp"
#include <hls_math.h>

void determine_page_split_location(Page page1, const int &freePageIndex, PageSplit &pageSplit);
void split_page(Page page1, Page page2, const PageSplit &pageSplit);


void page_splitter(hls::stream_of_blocks<IPage> &pageInS, hls::stream_of_blocks<Page> pageOutS[2], hls::stream<PageProperties> &pagePropertyStream)
{
    if(!pageInS.empty()){
        hls::read_lock<IPage> pageIn(pageInS);
        hls::write_lock<Page> page1(pageOutS[0]);
        for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
            #pragma HLS PIPELINE II=1
            page1[n] = pageIn[n];
        }
        PageProperties p = rawToProperties<PageProperties>(pageIn[MAX_NODES_PER_PAGE]);
        
        if(p.splitPage){
            PageSplit pageSplit;
            determine_page_split_location(page1, p.freePageIdx, pageSplit);
            hls::write_lock<Page> page2(pageOutS[1]);
            for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
                page2[n] = 0;
            }
            
            split_page(page1, page2, pageSplit);
            p.extraPage = true;
        }
        pagePropertyStream.write(p);
    }
}


void split_page(Page page1, Page page2, const PageSplit &pageSplit)
{
    int stack[MAX_NODES_PER_PAGE];
    int stack_ptr = 0;
    stack[stack_ptr] = pageSplit.bestSplitLocation;

    split_page_loop: for(int i = 0; i < pageSplit.nrOfBranchedNodes; i++){
        #pragma HLS LOOP_TRIPCOUNT max=MAX_NODES_PER_PAGE min=1 avg=PAGE_SPLIT_TARGET
        #pragma HLS PIPELINE II=6
        Node_hbm node(rawToNode(page1[stack[i]]));
        if(node.idx() == pageSplit.bestSplitLocation){
            node.idx(0);
        }
        
        if(!node.leaf()){
            if(!node.leftChild.isPage()){
                stack[++stack_ptr] = node.leftChild.id();
            }
            if(!node.rightChild.isPage()){
                stack[++stack_ptr] = node.rightChild.id();
            }
        }
        page2[node.idx()] = nodeToRaw(node);
        node.valid(false); //Set node to invalid
        page1[stack[i]] = nodeToRaw(node);
    }
}

void determine_page_split_location(Page page1, const int &freePageIndex, PageSplit &pageSplit)
{
    int stack[MAX_NODES_PER_PAGE];
    int stack_ptr = 0;
    stack[stack_ptr] = 0;

    bool processed[MAX_NODES_PER_PAGE];
    int descendant_count[MAX_NODES_PER_PAGE];
    init_determine: for(int i = 0; i < MAX_NODES_PER_PAGE;i++){
        #pragma HLS PIPELINE II=1
        processed[i] = false;
        descendant_count[i] = 1;
    }

    int parentIdx[MAX_NODES_PER_PAGE];

    map_tree: while(stack_ptr >= 0){
        #pragma HLS LOOP_TRIPCOUNT max=MAX_TREE_MAP_ITER min=1
        #pragma HLS PIPELINE II=5
        Node_hbm node(rawToNode(page1[stack[stack_ptr]]));
        if(!node.leaf()){
            if(!node.leftChild.isPage() && !processed[node.leftChild.id()]){
                stack[++stack_ptr] = node.leftChild.id();
                parentIdx[node.leftChild.id()] = node.idx();
                processed[node.leftChild.id()] = true;
            } else if(!node.rightChild.isPage() && !processed[node.rightChild.id()]){
                stack[++stack_ptr] = node.rightChild.id();
                parentIdx[node.rightChild.id()] = node.idx();
                processed[node.rightChild.id()] = true;
            } else{
                if(!node.leftChild.isPage()){
                    descendant_count[node.idx()] += descendant_count[node.leftChild.id()];
                }
                if(!node.rightChild.isPage()){
                    descendant_count[node.idx()] += descendant_count[node.rightChild.id()];
                }
                processed[node.idx()] = true;
                stack_ptr--;
            }
        } else{
            processed[node.idx()] = true;
            stack_ptr--;
        }
    }

    pageSplit.bestSplitValue = MAX_NODES_PER_PAGE;
    find_split_value: for(int i=1; i < MAX_NODES_PER_PAGE; i++){
        #pragma HLS PIPELINE II=2
        int diff = hls::abs(PAGE_SPLIT_TARGET - descendant_count[i]);
        if(diff < pageSplit.bestSplitValue){
            pageSplit.bestSplitValue = diff;
            pageSplit.bestSplitLocation = i;
        }
    }
    pageSplit.nrOfBranchedNodes = descendant_count[pageSplit.bestSplitLocation];
    //Update parent of splitter
    Node_hbm parent(rawToNode(page1[parentIdx[pageSplit.bestSplitLocation]]));
    if(parent.leftChild.id() == pageSplit.bestSplitLocation){
        parent.leftChild.isPage(true);
        parent.leftChild.id(freePageIndex);
    }else{
        parent.rightChild.isPage(true);
        parent.rightChild.id(freePageIndex);
    }
    page1[parent.idx()] = nodeToRaw(parent);
}