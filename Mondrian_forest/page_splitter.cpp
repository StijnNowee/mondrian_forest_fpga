#include "train.hpp"

bool find_free_nodes(PageProperties &p, hls::write_lock<IPage> &out);
PageSplit determine_page_split_location(hls::write_lock<IPage> &out, int freePageIndex);
void split_page(hls::write_lock<IPage> &out, IPage &newPage, PageSplit pageSplit, PageProperties &p);

void page_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream_of_blocks<IPage> &pageOut, const int loopCount, hls::stream<bool> &treeDoneStream)
{
    bool saveExtraPage = false;
    IPage newPage;
    static int freePageIndex[TREES_PER_BANK] = {0};

    main_loop: for(int iter = 0; iter < loopCount;){
        if(!pageIn.empty()){
            hls::write_lock<IPage> out(pageOut);
            if(saveExtraPage){
                save_new_page: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
                    out[i] = newPage[i];
                }
                auto p = convertProperties(newPage[MAX_NODES_PER_PAGE]);
                find_free_nodes(p, out);
                out[MAX_NODES_PER_PAGE] = convertProperties(p);
                saveExtraPage = false;
            }else{
                hls::read_lock<IPage> in(pageIn);
                save_to_output: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
                    out[i] = in[i];
                }
                auto p = convertProperties(in[MAX_NODES_PER_PAGE]);
                
                if(p.split.enabled){
                    if(!find_free_nodes(p, out)){
                        PageSplit pageSplit = determine_page_split_location(out, ++freePageIndex[p.treeID]);
                        split_page(out, newPage, pageSplit, p);
                        find_free_nodes(p, out);
                        p.dontIterate = true;
                        saveExtraPage = true;
                    }
                }
                out[MAX_NODES_PER_PAGE] = convertProperties(p);
            }
        }
        #if(defined __SYNTHESIS__)
            for(;!treeDoneStream.empty();){
                treeDoneStream.read();
                iter++;
            }
        #else
            if(!saveExtraPage){
                iter++;
            }
        #endif
    }
}

bool find_free_nodes(PageProperties &p, hls::write_lock<IPage> &out)
{
    int index1 = -1, index2 = -1;
    for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        if(!out[n].get_bit(33)){
            if(index1 == -1){
                index1 = n;
            }else{
                index2 = n;
            }
        };
    }
    p.freeNodesIdx[0] = index1;
    if(index2 == -1){
        return false;
    }else{
        p.freeNodesIdx[1] = index2;
        return true;
    }
}

void split_page(hls::write_lock<IPage> &out, IPage &newPage, PageSplit pageSplit, PageProperties &p)
{
    int stack[MAX_NODES_PER_PAGE];
    int stack_ptr = 0;
    stack[stack_ptr] = pageSplit.bestSplitLocation;
    Node_hbm node; //= convertNode(out[pageSplit.bestSplitLocation]);
    node.idx = 0;
    PageProperties newP;
    newP.split = p.split;
    newP.treeID = p.treeID;
    newP.pageIdx = pageSplit.freePageIndex;

    p.split.enabled = false;
    for(int i = 0; i < pageSplit.nrOfBranchedNodes; i++){
        node = convertNode(out[stack[i]]);
        if(!node.leaf){
            if(!node.leftChild.isPage){
                stack[++stack_ptr] = node.leftChild.id;
            }
            if(!node.rightChild.isPage){
                stack[++stack_ptr] = node.rightChild.id;
            }
        }
        if(stack[i] == p.split.nodeIdx){
            newP.split.enabled = true;
            p.split.enabled = false;
        }
        newPage[node.idx] = convertNode(node);
        out[stack[i]].set_bit(33, false); //Set node to invalid
    }
    newPage[MAX_NODES_PER_PAGE] = convertProperties(newP);
    std::cout << "After split: " << newP.treeID << std::endl; 
}

PageSplit determine_page_split_location(hls::write_lock<IPage> &out, int freePageIndex)
{
    int stack[MAX_NODES_PER_PAGE];
    int stack_ptr = 0;
    stack[stack_ptr] = 0;

    bool processed[MAX_NODES_PER_PAGE];
    int descendant_count[MAX_NODES_PER_PAGE];
    for(int i = 0; i < MAX_NODES_PER_PAGE;i++){
        processed[i] = false;
        descendant_count[i] = 1;
    }

    Node_hbm node;
    ChildNode leftChild, rightChild;
    int parentIdx[MAX_NODES_PER_PAGE];

    for(int i = 0; i < MAX_ITERATION; i++){
        if(stack_ptr >= 0) {
            node = convertNode(out[stack[stack_ptr]]);
            if(!node.leaf){
                leftChild = node.leftChild;
                rightChild = node.rightChild;
                if(!leftChild.isPage && !processed[leftChild.id]){
                    stack[++stack_ptr] = leftChild.id;
                    parentIdx[leftChild.id] = node.idx;
                } else if(!rightChild.isPage && !processed[rightChild.id]){
                    stack[++stack_ptr] = rightChild.id;
                    parentIdx[rightChild.id] = node.idx;
                } else{
                    if(!leftChild.isPage){
                        descendant_count[node.idx] += descendant_count[leftChild.id];
                    }
                    if(!rightChild.isPage){
                        descendant_count[node.idx] += descendant_count[rightChild.id];
                    }
                    processed[node.idx] = true;
                    stack_ptr--;
                }
            } else{
                processed[node.idx] = true;
                stack_ptr--;
            }
        }
    }

    PageSplit pageSplit;

    for(int i=0; i < MAX_NODES_PER_PAGE; i++){
        int diff = abs(PAGE_SPLIT_TARGET - descendant_count[i]);
        if(diff < pageSplit.bestSplitValue){
            pageSplit.bestSplitValue = diff;
            pageSplit.bestSplitLocation = i;
        }
    }
    pageSplit.nrOfBranchedNodes = descendant_count[pageSplit.bestSplitLocation];
    pageSplit.freePageIndex = freePageIndex;
    //Update parent of splitter
    auto parent = convertNode(out[parentIdx[pageSplit.bestSplitLocation]]);
    if(parent.leftChild.id == pageSplit.bestSplitLocation){
        parent.leftChild.isPage = true;
        parent.leftChild.id = freePageIndex;
    }else{
        parent.rightChild.isPage = true;
        parent.rightChild.id = freePageIndex;
    }
    return pageSplit;
}