#include "train.hpp"
#include <cwchar>
#include "converters.hpp"
#include "hls_math.h"

bool find_free_nodes(PageProperties &p, const IPage localPage);
void determine_page_split_location(IPage page1, int freePageIndex, PageSplit &pageSplit);
void split_page(IPage page1, IPage page2, const PageSplit &pageSplit, PageProperties &p, PageProperties &newP);


void page_splitter(hls::stream_of_blocks<IPage> pageInS[TRAVERSAL_BLOCKS], hls::stream_of_blocks<IPage> &pageOut1S, hls::stream_of_blocks<IPage> &pageOut2S)
{
    static int j = 0;

    if(!pageInS[j].empty()){
    
    hls::read_lock<IPage> pageIn(pageInS[j]);
    hls::write_lock<IPage> page1(pageOut1S);
    hls::write_lock<IPage> page2(pageOut2S);
    PageProperties p1 = rawToProperties(pageIn[MAX_NODES_PER_PAGE]);
    j = (++j == TRAVERSAL_BLOCKS) ? 0 : j;
    for(int n = 0; n < MAX_NODES_PER_PAGE + 1; n++){
        page1[n] = pageIn[n];
        page2[n] = 0;
    }
    if(p1.split.enabled){
        if(!find_free_nodes(p1, pageIn)){
            if(p1.freePageIdx != MAX_PAGES_PER_TREE){
                PageProperties p2;
                PageSplit pageSplit;
                determine_page_split_location(page1, p1.freePageIdx, pageSplit);
                split_page(page1, page2, pageSplit, p1, p2);
                if(p1.split.enabled){
                    find_free_nodes(p1, page1);
                }else{
                    find_free_nodes(p2, page2);
                }
                p2.extraPage = true;
                page2[MAX_NODES_PER_PAGE] = propertiesToRaw(p2);

            }else{
                p1.split.enabled = false;
            }
        }
    }
    page1[MAX_NODES_PER_PAGE] = propertiesToRaw(p1);
    }
}


bool find_free_nodes(PageProperties &p, const IPage localPage)
{
    #pragma HLS inline off
    int index1 = 255, index2 = 255;
    find_free_nodes: for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        #pragma HLS PIPELINE II=1
        auto node = rawToNode(localPage[n]);
        if(!node.valid()){
            if(index1 == 255){
                index1 = n;
            }else if(index2 == 255){
                index2 = n;
            }
        };
    }
    
    if(index2 == 255){
        return false;
    }else{
        p.freeNodesIdx[0] = index1;
        p.freeNodesIdx[1] = index2;
        return true;
    }
}

void split_page(IPage page1, IPage page2, const PageSplit &pageSplit, PageProperties &p, PageProperties &newP)
{
    int stack[MAX_NODES_PER_PAGE];
    int stack_ptr = 0;
    stack[stack_ptr] = pageSplit.bestSplitLocation;
    if(p.split.parentIdx == pageSplit.bestSplitLocation){
        p.split.parentIdx = 0;
    }
    
    newP.split = p.split;
    newP.treeID = p.treeID;
    newP.pageIdx = pageSplit.freePageIndex;
    newP.shouldSave = true;

    newP.split.enabled = false;
    split_page_loop: for(int i = 0; i < pageSplit.nrOfBranchedNodes; i++){
        #pragma HLS PIPELINE II=4
        Node_hbm node(rawToNode(page1[stack[i]]));
        if(p.split.nodeIdx == node.idx()){
            newP.split.enabled = true;
            p.split.enabled = false;
        }
        if(node.idx() == pageSplit.bestSplitLocation){
            if(p.split.nodeIdx == node.idx()){
                newP.split.nodeIdx = 0;
            }
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

void determine_page_split_location(IPage page1, int freePageIndex, PageSplit &pageSplit)
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
    find_split_value: for(int i=0; i < MAX_NODES_PER_PAGE; i++){
        int diff = hls::abs(PAGE_SPLIT_TARGET - descendant_count[i]);
        if(diff < pageSplit.bestSplitValue){
            pageSplit.bestSplitValue = diff;
            pageSplit.bestSplitLocation = i;
        }
    }
    pageSplit.nrOfBranchedNodes = descendant_count[pageSplit.bestSplitLocation];
    pageSplit.freePageIndex = freePageIndex;
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