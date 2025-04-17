#include "train.hpp"
#include "converters.hpp"
#include "processing_unit.hpp"

void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, const int freeNodeIdx);
void split_node(IPage page, const PageProperties &p);
bool find_free_nodes(const IPage page, PageProperties &p);
void set_new_leaf_weight(Node_hbm &leaf);

void node_splitter(hls::stream_of_blocks<IPage> pageInS[TRAIN_TRAVERSAL_BLOCKS], hls::stream_of_blocks<IPage> &pageOutS, const int &blockIdx)
{
    int traverseBlockId = TRAIN_TRAVERSAL_BLOCKS;
    for(int b = 0; b < TRAIN_TRAVERSAL_BLOCKS; b++){
        #pragma HLS PIPELINE II=1
        int idx =  (b + blockIdx) % 3;
        if(!pageInS[idx].empty()){
            
            traverseBlockId = idx;
        }
    }
    if(traverseBlockId != TRAIN_TRAVERSAL_BLOCKS){
        hls::read_lock<IPage> pageIn(pageInS[traverseBlockId]);
        hls::write_lock<IPage> pageOut(pageOutS);
        PageProperties p = rawToProperties<PageProperties>(pageIn[MAX_NODES_PER_PAGE]);
        for (int n = 0; n < MAX_NODES_PER_PAGE; n++) {
            #pragma HLS PIPELINE II=1
            pageOut[n] = pageIn[n];
        }
        if(p.split.enabled && find_free_nodes(pageOut, p)){
            split_node(pageOut, p);
        }
        pageOut[MAX_NODES_PER_PAGE] = propertiesToRaw(p);
        
    }
}


void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, const int freeNodeIdx)
{
    if(currentNode.idx() == 0){
        //New root node
        currentNode.idx(freeNodeIdx);
        newNode.idx(0);

    }else{
        //Add node to array
        newNode.idx(freeNodeIdx);
    }
}

void split_node(IPage page, const PageProperties &p){
    Node_hbm node = rawToNode(page[p.split.nodeIdx]);

    auto featureValue = p.input.feature[p.split.dimension]; 
    unit_interval upperBound = node.lowerBound[p.split.dimension], lowerBound = node.upperBound[p.split.dimension]; //Intended
    //Seems strange but saves a operation
    if(featureValue > lowerBound){
        upperBound = featureValue;
    }else{
        lowerBound = featureValue;
    }

    //Create the two new nodes
    Node_hbm newNode(p.split.dimension, 
                    p.split.newSplitTime, 
                    node.parentSplitTime,
                    lowerBound + p.split.rngVal * (upperBound - lowerBound), 
                    false, 0);

    assign_node_idx(node, newNode, p.freeNodesIdx[0]);
    
    //Update tabs

    Node_hbm newSibbling(p.input.label, 
                        MAX_LIFETIME,
                        newNode.splittime, 
                        0, 
                        true, 
                        p.freeNodesIdx[1]);
    
    newSibbling.counts[p.input.label] = 1;
    set_new_leaf_weight(newSibbling);
    #ifdef BINARY 
    Directions dir = !(p.input.feature[p.split.dimension]) ? LEFT : RIGHT;
    #else
    Directions dir = (p.input.feature[p.split.dimension] <= newNode.threshold) ? LEFT : RIGHT;
    #endif
    
    set_tab: for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        if(node.counts[c] > 0){
            newNode.setTab(dir, c);
        }
    }
    newNode.setTab((dir == LEFT) ? RIGHT : LEFT, p.input.label);
    update_weight(newNode);
    //New lower and upper bounds
    update_bounds: for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        #pragma HLS PIPELINE II=1
        auto feature = p.input.feature[d];

        newNode.lowerBound[d] = (node.lowerBound[d] > feature) ? feature : node.lowerBound[d];
        newNode.upperBound[d] = (feature > node.upperBound[d]) ? feature : node.upperBound[d];
        newSibbling.lowerBound[d] = feature;
        newSibbling.upperBound[d] = feature;
    }

    if(dir == LEFT){
        newNode.leftChild = ChildNode(false, newSibbling.idx());
        newNode.rightChild = ChildNode(false, node.idx());
    }else{
        newNode.leftChild = ChildNode(false, node.idx());
        newNode.rightChild = ChildNode(false, newSibbling.idx());
    };
    node.parentSplitTime = p.split.newSplitTime;
    Node_hbm parent(rawToNode(page[p.split.parentIdx]));
    if(p.split.nodeIdx != 0){
        
        //Update connections of other nodes
        if(!parent.leftChild.isPage() && parent.leftChild.id() == node.idx()){
            parent.leftChild.id(newNode.idx());
        }else{
            parent.rightChild.id(newNode.idx());
        }
        page[parent.idx()] = nodeToRaw(parent);
    }

    //Write new node
    page[node.idx()] = nodeToRaw(node);
    page[newNode.idx()] = nodeToRaw(newNode);
    page[newSibbling.idx()] = nodeToRaw(newSibbling);
}

bool find_free_nodes(const IPage page, PageProperties &p)
{
    int nrOfFreeNodes = 0;
    bool node1Set = false, node2Set = false;
    find_free_nodes: for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        #pragma HLS PIPELINE II=1
        auto node = rawToNode(page[n]);
        if(!node.valid()){
            nrOfFreeNodes++;
            if(!node1Set){
                p.freeNodesIdx[0] = n;
                node1Set = true;
            }else if(!node2Set){
                p.freeNodesIdx[1] = n;
                node2Set = true;
            }
        };
    }
    if(nrOfFreeNodes >= 2){
        if(nrOfFreeNodes < 4 && p.freePageIdx < MAX_PAGES_PER_TREE){
            p.splitPage = true;
        }
        return true;
    }else{
        return false;
    }
}

void set_new_leaf_weight(Node_hbm &leaf)
{
    static const ap_ufixed<9,1> tmpDivision = ap_ufixed<9,1>(1.0) / (1 + ap_ufixed<8,7>(BETA));
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        leaf.weight[c] = (leaf.counts[c] + unit_interval(ALPHA)) *tmpDivision;
    }
}