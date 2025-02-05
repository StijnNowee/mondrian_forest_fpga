#include "train.hpp"

bool find_free_nodes(PageProperties &p, hls::write_lock<IPage> &out);
PageSplit determine_page_split_location(hls::write_lock<IPage> &out);
void split_page(hls::write_lock<IPage> &out, IPage &newPage, PageSplit pageSplit, PageProperties &p, int freePageIndex);

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
                auto p = convertRawToProperties(newPage[MAX_NODES_PER_PAGE]);
                find_free_nodes(p, out);
                out[MAX_NODES_PER_PAGE] = convertPropertiesToRaw(p);
                saveExtraPage = false;
            }else{
                hls::read_lock<IPage> in(pageIn);
                save_to_output: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
                    out[i] = in[i];
                }
                auto p = convertRawToProperties(in[MAX_NODES_PER_PAGE]);
                
                if(p.split.enabled){
                    if(!find_free_nodes(p, out)){
                        PageSplit pageSplit = determine_page_split_location(out);
                        split_page(out, newPage, pageSplit, p, ++freePageIndex[p.treeID]);
                        find_free_nodes(p, out);
                        p.dontIterate = true;
                        saveExtraPage = true;
                    }
                }
                out[MAX_NODES_PER_PAGE] = convertPropertiesToRaw(p);
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
    node_converter node_conv;
    int index1 = -1, index2 = -1;
    for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        node_conv.raw = out[n];
        if(!node_conv.node.valid){
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

void split_page(hls::write_lock<IPage> &out, IPage &newPage, PageSplit pageSplit, PageProperties &p, int freePageIndex)
{
    int stack[MAX_NODES_PER_PAGE];
    int stack_ptr = 0;
    stack[stack_ptr] = pageSplit.bestSplitLocation;
    node_converter conv(out[pageSplit.bestSplitLocation]);
    conv.node.idx = 0;
    PageProperties newP;
    p.split = p.split;
    p.treeID = p.treeID;
    //-------------CHANGE LATER----------------
    p.pageIdx = freePageIndex;
    //-----------------------------------------

    p.split.enabled = false;
    for(int i = 0; i < pageSplit.nrOfBranchedNodes; i++){
        conv.raw = out[stack[i]];
        if(!conv.node.leaf){
            if(!conv.node.leftChild.isPage){
                stack[++stack_ptr] = conv.node.leftChild.id;
            }
            if(!conv.node.rightChild.isPage){
                stack[++stack_ptr] = conv.node.rightChild.id;
            }
        }
        if(stack[i] == p.split.nodeIdx){
            p.split.enabled = true;
            p.split.enabled = false;
        }
        newPage[conv.node.idx] = conv.raw;
        conv.node.valid = false;
    }
    newPage[MAX_NODES_PER_PAGE] = convertPropertiesToRaw(newP);
    std::cout << "After split: " << newP.treeID << std::endl; 
}

PageSplit determine_page_split_location(hls::write_lock<IPage> &out)
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
 
    node_converter conv;
    ChildNode leftChild, rightChild;

    for(int i = 0; i < MAX_ITERATION; i++){
        if(stack_ptr >= 0) {
            conv.raw = out[stack[stack_ptr]];
            if(!conv.node.leaf){
                leftChild = conv.node.leftChild;
                rightChild = conv.node.rightChild;
                if(!leftChild.isPage && !processed[leftChild.id]){
                    stack[++stack_ptr] = leftChild.id;
                } else if(!rightChild.isPage && !processed[rightChild.id]){
                    stack[++stack_ptr] = rightChild.id;
                } else{
                    if(!leftChild.isPage){
                        descendant_count[conv.node.idx] += descendant_count[leftChild.id];
                    }
                    if(!rightChild.isPage){
                        descendant_count[conv.node.idx] += descendant_count[rightChild.id];
                    }
                    processed[conv.node.idx] = true;
                    stack_ptr--;
                }
            } else{
                processed[conv.node.idx] = true;
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
    return pageSplit;
}