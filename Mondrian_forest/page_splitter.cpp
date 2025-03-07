#include "train.hpp"
#include <cwchar>

bool find_free_nodes(PageProperties &p, IPage &localPage);
PageSplit determine_page_split_location(IPage &inputPage, int freePageIndex);
void split_page(IPage &inputPage, IPage &newPage, const PageSplit &pageSplit, PageProperties &p, PageProperties &newP);

// void page_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream_of_blocks<IPage> &pageOut)
// {
//     static bool saveExtraPage = false;
//     IPage inputPage = {};
//     IPage extraPage = {};

//     static int freePageIndex[TREES_PER_BANK] = {0};

//     if(saveExtraPage || !pageIn.empty()){
        
    
//         if(saveExtraPage){
            
//             PageProperties p;
//             convertRawToProperties(localPage[MAX_NODES_PER_PAGE], p);
            
//             find_free_nodes(p, localPage);
//             //convertPropertiesToRaw(p, localPage[MAX_NODES_PER_PAGE]);
//             saveExtraPage = false;
//             std::cout << "PageSplitter Extra: " << p.split.nodeIdx << std::endl;
//         }else if (!pageIn.empty()){
//             hls::read_lock<IPage> in(pageIn);
//             PageProperties p;
//             convertRawToProperties(in[MAX_NODES_PER_PAGE], p);
//             std::cout << "PageSplitter standard before: " << p.split.nodeIdx << std::endl;
            
//             if(p.split.enabled){
//                 if(!find_free_nodes(p, localPage)){
//                     if(freePageIndex[p.treeID] != MAX_PAGES_PER_TREE){
//                         for(int i =0; i < MAX_NODES_PER_PAGE; i++){
//                             localPage[i] = 0;
//                         }
//                         PageSplit pageSplit = determine_page_split_location(localPage, ++freePageIndex[p.treeID]);
//                         split_page(localPage, newPage, pageSplit, p);
//                         find_free_nodes(p, out);
//                         p.extraPage = true;
//                         saveExtraPage = true;
//                     }else{
//                         p.split.enabled = false;
//                     }
//                 }
//             }
//             hls::write_lock<IPage> out(pageOut);
//             for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
//                 out[i] = localPage[i];
//             }
//             std::cout << "PageSplitter standard: " << p.split.nodeIdx << std::endl;
//             convertPropertiesToRaw(p, out[MAX_NODES_PER_PAGE]);
//         }
//     }
//     // #if(defined __SYNTHESIS__)
//     //     check_tree_done: for(;!treeDoneStream.empty();){
//     //         treeDoneStream.read();
//     //         iter++;
//     //     }
//     // #else
//     //     if(!saveExtraPage){
//     //         iter++;
//     //     }
//     // #endif
//     // }
// }

void page_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream_of_blocks<IPage> &pageOut)
{
    
    static int freePageIndex[TREES_PER_BANK] = {0};
    if(!pageIn.empty()){
    IPage inputPage = {};
    PageProperties p;
    read_page(inputPage, p, pageIn);
    if(p.split.enabled){
        if(!find_free_nodes(p, inputPage)){
            if(freePageIndex[p.treeID] != MAX_PAGES_PER_TREE){
                IPage newPage = {};
                PageProperties newP;
                PageSplit pageSplit = determine_page_split_location(inputPage, ++freePageIndex[p.treeID]);
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


bool find_free_nodes(PageProperties &p, IPage &localPage)
{
    int index1 = 255, index2 = 255;
    find_free_nodes: for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        // Node_hbm node;
        // convertRawToNode(out[n], node);
        // memcpy(&node, &out[n], sizeof(Node_hbm));
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
    Node_hbm node;
    node.idx = 0;
    newP.split = p.split;
    newP.treeID = p.treeID;
    newP.pageIdx = pageSplit.freePageIndex;

    newP.split.enabled = false;
    split_page_loop: for(int i = 0; i < pageSplit.nrOfBranchedNodes; i++){
        convertRawToNode(inputPage[stack[i]], node);
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
        inputPage[stack[i]] = 0; //Set node to invalid
    }
    convertPropertiesToRaw( newP, newPage[MAX_NODES_PER_PAGE]); 
}

PageSplit determine_page_split_location(IPage &inputPage, int freePageIndex)
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
            convertRawToNode(inputPage[stack[stack_ptr]], node);
            if(!node.leaf){
                leftChild = node.leftChild;
                rightChild = node.rightChild;
                if(!leftChild.isPage && !processed[leftChild.id]){
                    stack[++stack_ptr] = leftChild.id;
                    parentIdx[leftChild.id] = node.idx;
                    processed[leftChild.id] = true;
                } else if(!rightChild.isPage && !processed[rightChild.id]){
                    stack[++stack_ptr] = rightChild.id;
                    parentIdx[rightChild.id] = node.idx;
                    processed[rightChild.id] = true;
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
    convertRawToNode(inputPage[parentIdx[pageSplit.bestSplitLocation]], parent);
    if(parent.leftChild.id == pageSplit.bestSplitLocation){
        parent.leftChild.isPage = true;
        parent.leftChild.id = freePageIndex;
    }else{
        parent.rightChild.isPage = true;
        parent.rightChild.id = freePageIndex;
    }
    // memcpy(&out[parent.idx], &parent, sizeof(Node_hbm));
    convertNodeToRaw(parent, inputPage[parent.idx]);
    return pageSplit;
}