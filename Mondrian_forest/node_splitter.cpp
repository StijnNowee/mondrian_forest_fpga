#include "train.hpp"
#include <cwchar>
#include "converters.hpp"

void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, const int freeNodeIdx);
void split_node(IPage page, const PageProperties &p);
bool find_free_nodes(const IPage page, PageProperties &p);

void node_splitter(hls::stream_of_blocks<IPage> pageInS[TRAVERSAL_BLOCKS], hls::stream_of_blocks<IPage> &pageOutS, const int &blockIdx)
{
    int traverseBlockId = TRAVERSAL_BLOCKS;
    for(int b = 0; b < TRAVERSAL_BLOCKS; b++){
        int idx =  (b + blockIdx) % 3;
        if(!pageInS[idx].empty()){
            
            traverseBlockId = idx;
        }
    }
    if(traverseBlockId != TRAVERSAL_BLOCKS){
        hls::read_lock<IPage> pageIn(pageInS[traverseBlockId]);
        hls::write_lock<IPage> pageOut(pageOutS);
        PageProperties p = rawToProperties(pageIn[MAX_NODES_PER_PAGE]);
        for (int n = 0; n < MAX_NODES_PER_PAGE; n++) {
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

                    // newNode.idx = (p.split.nodeIdx == 0) ? 0 : p.freeNodesIdx[0];
                    // node.idx = (p.split.nodeIdx == 0) ? p.freeNodesIdx[0] : node.idx;
    assign_node_idx(node, newNode, p.freeNodesIdx[0]);

    //Update tabs

    Node_hbm newSibbling(p.input.label, 
                        MAX_LIFETIME,
                        newNode.splittime, 
                        0, 
                        true, 
                        p.freeNodesIdx[1]);
    
    newSibbling.counts[p.input.label] = 1;
    Directions dir = (p.input.feature[p.split.dimension] <= newNode.threshold) ? LEFT : RIGHT;
    newNode.setTab((dir == LEFT) ? RIGHT : LEFT, p.input.label);
    
    //New lower and upper bounds
    update_bounds: for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        #pragma HLS PIPELINE II=1
        auto feature = p.input.feature[d];

        newNode.lowerBound[d] = (node.lowerBound[d] > feature) ? feature : node.lowerBound[d];
        newNode.upperBound[d] = (feature > node.upperBound[d]) ? feature : node.upperBound[d];
        newSibbling.lowerBound[d] = feature;
        newSibbling.upperBound[d] = feature;
    }

    set_tab: for(int c = 0; c < CLASS_COUNT; c++){
        if(node.counts[c] > 0){
            newNode.setTab(dir, c);
        }
    }

    if(dir == LEFT){
        newNode.leftChild = ChildNode(false, newSibbling.idx());
        newNode.rightChild = ChildNode(false, node.idx());
    }else{
        newNode.leftChild = ChildNode(false, node.idx());
        newNode.rightChild = ChildNode(false, newSibbling.idx());
    };
    node.parentSplitTime = p.split.newSplitTime;

    if(p.split.nodeIdx != 0){
        Node_hbm parent(rawToNode(page[p.split.parentIdx]));
        update_internal_posterior_predictive_distribution(newNode, parent.posteriorP);
        //Update connections of other nodes
        if(!parent.leftChild.isPage() && parent.leftChild.id() == node.idx()){
            parent.leftChild.id(newNode.idx());
        }else{
            parent.rightChild.id(newNode.idx());
        }
        page[parent.idx()] = nodeToRaw(parent);
    }

    update_leaf_posterior_predictive_distribution(newSibbling, newNode.posteriorP);

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