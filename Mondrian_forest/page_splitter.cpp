#include "train.hpp"
#include <cwchar>

bool find_free_nodes(PageProperties &p, hls::write_lock<IPage> &out);
PageSplit determine_page_split_location(hls::write_lock<IPage> &out, int freePageIndex);
void split_page(hls::write_lock<IPage> &out, IPage &newPage, const PageSplit &pageSplit, PageProperties &p);

void page_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream_of_blocks<IPage> &pageOut)
{
    static bool saveExtraPage = false;
    static IPage newPage = {};
    static int freePageIndex[TREES_PER_BANK] = {0};

    if(saveExtraPage || !pageIn.empty()){
        hls::write_lock<IPage> out(pageOut);
    
        if(saveExtraPage){
            save_new_page: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
                out[i] = newPage[i];
            }
            PageProperties p;
            convertRawToProperties(newPage[MAX_NODES_PER_PAGE], p);
            
            find_free_nodes(p, out);
            convertPropertiesToRaw(p, out[MAX_NODES_PER_PAGE]);
            saveExtraPage = false;
        }else if (!pageIn.empty()){
            hls::read_lock<IPage> in(pageIn);
            save_to_output: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
                out[i] = in[i];
            }
            PageProperties p;
            convertRawToProperties(in[MAX_NODES_PER_PAGE], p);
            
            if(p.split.enabled){
                if(!find_free_nodes(p, out)){
                    if(freePageIndex[p.treeID] != MAX_PAGES_PER_TREE){
                        for(int i =0; i < MAX_NODES_PER_PAGE; i++){
                            newPage[i] = 0;
                        }
                        PageSplit pageSplit = determine_page_split_location(out, ++freePageIndex[p.treeID]);
                        split_page(out, newPage, pageSplit, p);
                        find_free_nodes(p, out);
                        p.extraPage = true;
                        saveExtraPage = true;
                    }else{
                        p.split.enabled = false;
                    }
                }
            }
            convertPropertiesToRaw(p, out[MAX_NODES_PER_PAGE]);
        }
    }
    // #if(defined __SYNTHESIS__)
    //     check_tree_done: for(;!treeDoneStream.empty();){
    //         treeDoneStream.read();
    //         iter++;
    //     }
    // #else
    //     if(!saveExtraPage){
    //         iter++;
    //     }
    // #endif
    // }
}

bool find_free_nodes(PageProperties &p, hls::write_lock<IPage> &out)
{
    int index1 = 255, index2 = 255;
    find_free_nodes: for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        // Node_hbm node;
        // convertRawToNode(out[n], node);
        // memcpy(&node, &out[n], sizeof(Node_hbm));
        if(out[n] == 0){
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

void split_page(hls::write_lock<IPage> &out, IPage &newPage, const PageSplit &pageSplit, PageProperties &p)
{
    int stack[MAX_NODES_PER_PAGE];
    int stack_ptr = 0;
    stack[stack_ptr] = pageSplit.bestSplitLocation;
    Node_hbm node;
    node.idx = 0;
    PageProperties newP;
    newP.split = p.split;
    newP.treeID = p.treeID;
    newP.pageIdx = pageSplit.freePageIndex;

    newP.split.enabled = false;
    split_page_loop: for(int i = 0; i < pageSplit.nrOfBranchedNodes; i++){
        convertRawToNode(out[stack[i]], node);
        //memcpy(&node, &out[stack[i]], sizeof(Node_hbm));
        if(node.idx == pageSplit.bestSplitLocation){
            if(p.split.nodeIdx == pageSplit.bestSplitLocation){
                newP.split.enabled = true;
                p.split.enabled = false;
                newP.split.nodeIdx = 0;
            }
            node.idx = 0;
        }
        if(!node.leaf){
            if(!node.leftChild.isPage){
                stack[++stack_ptr] = node.leftChild.id;
            }
            if(!node.rightChild.isPage){
                stack[++stack_ptr] = node.rightChild.id;
            }
        }
        // memcpy(&newPage[node.idx], &node, sizeof(Node_hbm));
        convertNodeToRaw(node, newPage[node.idx]);
        out[stack[i]] = 0; //Set node to invalid
    }
    convertPropertiesToRaw( newP, newPage[MAX_NODES_PER_PAGE]); 
}

PageSplit determine_page_split_location(hls::write_lock<IPage> &out, int freePageIndex)
{
    int stack[MAX_NODES_PER_PAGE];
    int stack_ptr = 0;
    stack[stack_ptr] = 0;

    bool processed[MAX_NODES_PER_PAGE];
    int descendant_count[MAX_NODES_PER_PAGE];
    init_determine: for(int i = 0; i < MAX_NODES_PER_PAGE;i++){
        processed[i] = false;
        descendant_count[i] = 1;
    }

    Node_hbm node;
    ChildNode leftChild, rightChild;
    int parentIdx[MAX_NODES_PER_PAGE];

    map_tree: for(int i = 0; i < MAX_ITERATION; i++){
        if(stack_ptr >= 0) {
            // memcpy(&node, &out[stack[stack_ptr]], sizeof(Node_hbm));
            convertRawToNode(out[stack[stack_ptr]], node);
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
    pageSplit.bestSplitValue = MAX_NODES_PER_PAGE;
    find_split_value: for(int i=0; i < MAX_NODES_PER_PAGE; i++){
        int diff = abs(PAGE_SPLIT_TARGET - descendant_count[i]);
        if(diff < pageSplit.bestSplitValue){
            pageSplit.bestSplitValue = diff;
            pageSplit.bestSplitLocation = i;
        }
    }
    pageSplit.nrOfBranchedNodes = descendant_count[pageSplit.bestSplitLocation];
    pageSplit.freePageIndex = freePageIndex;
    //Update parent of splitter
    Node_hbm parent;
    // memcpy(&parent, &out[parentIdx[pageSplit.bestSplitLocation]], sizeof(Node_hbm));
    convertRawToNode(out[parentIdx[pageSplit.bestSplitLocation]], parent);
    if(parent.leftChild.id == pageSplit.bestSplitLocation){
        parent.leftChild.isPage = true;
        parent.leftChild.id = freePageIndex;
    }else{
        parent.rightChild.isPage = true;
        parent.rightChild.id = freePageIndex;
    }
    // memcpy(&out[parent.idx], &parent, sizeof(Node_hbm));
    convertNodeToRaw(parent, out[parent.idx]);
    return pageSplit;
}