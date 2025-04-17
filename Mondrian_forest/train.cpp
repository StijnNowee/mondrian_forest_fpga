#include "train.hpp"
#include "processing_unit.hpp"
#include "converters.hpp"
void rng_splitter(hls::stream<unit_interval> &rngIn, hls::stream<unit_interval> &rngOut);
void train(hls::stream<FetchRequest> &fetchRequestStream, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], hls::stream<Feedback> &feedbackStream, Page *pageBank, const int &blockIdx)
{
    #pragma HLS DATAFLOW disable_start_propagation
    hls::stream_of_blocks<IPage, 3> fetchOut[TRAIN_TRAVERSAL_BLOCKS], traverseOut[TRAIN_TRAVERSAL_BLOCKS], nodeSplitOut;
    hls::stream_of_blocks<Page, 3> pageOut[2];
    hls::stream<PageProperties, 4> pagePropertyStream;
    
    fetcher<TRAIN_TRAVERSAL_BLOCKS, FetchRequest, PageProperties>(fetchRequestStream, fetchOut, pageBank, blockIdx);
    for(int i = 0; i < TRAIN_TRAVERSAL_BLOCKS; i++){
       #pragma HLS UNROLL
        tree_traversal(fetchOut[i], rngStream[i], traverseOut[i]);
    }
    
    node_splitter(traverseOut, nodeSplitOut, blockIdx);
    page_splitter(nodeSplitOut, pageOut, pagePropertyStream);
    save( pageOut, feedbackStream, pagePropertyStream, pageBank);
 
}

int checkValids(const IPage page)
{
    int validCount = 0;
    for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        if(rawToNode(page[n]).valid()) validCount++;
    }
    return validCount;
}

bool checkIdxLocations(const IPage page){
    for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        auto node = rawToNode(page[n]);
        if(node.valid() && node.idx() != n){
            return false;
        }
    }
    return true;
}

bool checkReachable(const int targetNumber, const IPage page)
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
        Node_hbm node(rawToNode(page[stack[stack_ptr]]));
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
    if(descendant_count[0] == targetNumber){
        return true;
    }else{
        return false;
    }
}

bool checkReachableTwo(const int targetNumber, const IPage page)
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
        Node_hbm node(rawToNode(page[stack[stack_ptr]]));
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
    if(descendant_count[0] == targetNumber){
        return true;
    }else{
        return false;
    }
}