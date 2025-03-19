#include "train.hpp"
#include <cwchar>
#include "converters.hpp"
#include "hls_math.h"

bool find_free_nodes(PageProperties &p, IPage &localPage);
void determine_page_split_location(IPage &inputPage, int freePageIndex, PageSplit &pageSplit);
void split_page(IPage &inputPage, IPage &newPage, const PageSplit &pageSplit, PageProperties &p, PageProperties &newP);


void page_splitter(hls::stream_of_blocks<IPage> pageIn[TRAVERSAL_BLOCKS], hls::stream_of_blocks<IPage> &pageOut)
{
    for(int block = 0; block < TRAVERSAL_BLOCKS; block++){
        if(!pageIn[block].empty()){
            IPage inputPage = {};
            PageProperties p;
            read_page(inputPage, p, pageIn[block]);
            if(p.split.enabled){
                if(!find_free_nodes(p, inputPage)){
                    if(p.freePageIdx != MAX_PAGES_PER_TREE){
                        IPage newPage = {};
                        PageProperties newP;
                        PageSplit pageSplit;
                        determine_page_split_location(inputPage, p.freePageIdx, pageSplit);
                        split_page(inputPage, newPage, pageSplit, p, newP);
                        if(p.split.enabled){
                            find_free_nodes(p, inputPage);
                        }else{
                            find_free_nodes(newP, newPage);
                        }
                        newP.extraPage = true;
                        write_page(newPage, newP, pageOut);

                    }else{
                        p.split.enabled = false;
                    }
                    
                }
            }
        write_page(inputPage, p, pageOut);
        }
    }
}


bool find_free_nodes(PageProperties &p, IPage &localPage)
{
    int index1 = 255, index2 = 255;
    find_free_nodes: for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        if(localPage[n] == 0){
            if(index1 == 255){
                index1 = n;
            }else{
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

void split_page(IPage &inputPage, IPage &newPage, const PageSplit &pageSplit, PageProperties &p, PageProperties &newP)
{
    int stack[MAX_NODES_PER_PAGE];
    int stack_ptr = 0;
    stack[stack_ptr] = pageSplit.bestSplitLocation;
    ;
    newP.split = p.split;
    newP.treeID = p.treeID;
    newP.pageIdx = pageSplit.freePageIndex;

    newP.split.enabled = false;
    split_page_loop: for(int i = 0; i < pageSplit.nrOfBranchedNodes; i++){
        #pragma HLS PIPELINE II=1
        Node_hbm node(rawToNode(inputPage[stack[i]]));
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
        newPage[node.idx()] = nodeToRaw(node);
        inputPage[stack[i]] = 0; //Set node to invalid
    }
    newPage[MAX_NODES_PER_PAGE] = propertiesToRaw(newP);
}

void determine_page_split_location(IPage &inputPage, int freePageIndex, PageSplit &pageSplit)
{
    int stack[MAX_NODES_PER_PAGE] = {0};
    int stack_ptr = 0;
    stack[stack_ptr] = 0;

    bool processed[MAX_NODES_PER_PAGE];
    int descendant_count[MAX_NODES_PER_PAGE];
    init_determine: for(int i = 0; i < MAX_NODES_PER_PAGE;i++){
        processed[i] = false;
        descendant_count[i] = 1;
    }

    ;
    ChildNode leftChild, rightChild;
    int parentIdx[MAX_NODES_PER_PAGE];

    map_tree: for(int i = 0; i < MAX_ITERATION; i++){
        if(stack_ptr >= 0) {
            Node_hbm node(rawToNode(inputPage[stack[stack_ptr]]));
            if(!node.leaf()){
                leftChild = node.leftChild;
                rightChild = node.rightChild;
                if(!leftChild.isPage() && !processed[leftChild.id()]){
                    stack[++stack_ptr] = leftChild.id();
                    parentIdx[leftChild.id()] = node.idx();
                    processed[leftChild.id()] = true;
                } else if(!rightChild.isPage() && !processed[rightChild.id()]){
                    stack[++stack_ptr] = rightChild.id();
                    parentIdx[rightChild.id()] = node.idx();
                    processed[rightChild.id()] = true;
                } else{
                    if(!leftChild.isPage()){
                        descendant_count[node.idx()] += descendant_count[leftChild.id()];
                    }
                    if(!rightChild.isPage()){
                        descendant_count[node.idx()] += descendant_count[rightChild.id()];
                    }
                    processed[node.idx()] = true;
                    stack_ptr--;
                }
            } else{
                processed[node.idx()] = true;
                stack_ptr--;
            }
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
    Node_hbm parent(rawToNode(inputPage[parentIdx[pageSplit.bestSplitLocation]]));
    if(parent.leftChild.id() == pageSplit.bestSplitLocation){
        parent.leftChild.isPage(true);
        parent.leftChild.id(freePageIndex);
    }else{
        parent.rightChild.isPage(true);
        parent.rightChild.id(freePageIndex);
    }
    inputPage[parent.idx()] = nodeToRaw(parent);
}