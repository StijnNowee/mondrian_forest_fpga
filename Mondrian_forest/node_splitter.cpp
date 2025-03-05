#include "train.hpp"
#include <cwchar>

void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, const int freeNodeIdx);

void node_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut)
{
    if(!pageIn.empty()){
    IPage localPage;
    PageProperties p;
        //Copy input
        
    read_page(localPage, p, pageIn);
    if(p.split.enabled){

        Node_hbm node;
        convertRawToNode(localPage[p.split.nodeIdx], node);

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
                        lowerBound + splitterRNGStream.read() * (upperBound - lowerBound), 
                        false, 0);

                        // newNode.idx = (p.split.nodeIdx == 0) ? 0 : p.freeNodesIdx[0];
                        // node.idx = (p.split.nodeIdx == 0) ? p.freeNodesIdx[0] : node.idx;

        assign_node_idx(node, newNode, p.freeNodesIdx[0]);

        Node_hbm newSibbling(p.input.label, 
                            MAX_LIFETIME,
                            newNode.splittime, 
                            0, 
                            true, 
                            p.freeNodesIdx[1]);
        newSibbling.labelCount++;
        newSibbling.classDistribution[p.input.label] = 1.0;
        
        //New lower and upper bounds
        for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
            #pragma HLS PIPELINE
            auto feature = p.input.feature[d];

            newNode.lowerBound[d] = (node.lowerBound[d] > feature) ? feature : node.lowerBound[d];
            newNode.upperBound[d] = (feature > node.upperBound[d]) ? feature : node.upperBound[d];
            newSibbling.lowerBound[d] = feature;
            newSibbling.upperBound[d] = feature;
        }

        if(p.input.feature[p.split.dimension] <= newNode.threshold){
            newNode.leftChild = ChildNode(false, newSibbling.idx);
            newNode.rightChild = ChildNode(false, node.idx);
        }else{
            newNode.leftChild = ChildNode(false, node.idx);
            newNode.rightChild = ChildNode(false, newSibbling.idx);
        };
        node.parentSplitTime = p.split.newSplitTime;

        if(p.split.nodeIdx != 0){
            Node_hbm parent;
            //memcpy(&parent, &out[p.split.parentIdx], sizeof(Node_hbm));
            convertRawToNode(localPage[p.split.parentIdx], parent);
            //Update connections of other nodes
            if(parent.leftChild.id == node.idx){
                parent.leftChild.id = newNode.idx;
            }else{
                parent.rightChild.id = newNode.idx;
            }
            convertNodeToRaw(parent, localPage[parent.idx]);
            //memcpy(&out[parent.idx], &parent, sizeof(Node_hbm));
        }

        //Write new node
        // memcpy(&out[node.idx], &node, sizeof(Node_hbm));
        // memcpy(&out[newNode.idx], &newNode, sizeof(Node_hbm));
        // memcpy(&out[newSibbling.idx], &newSibbling, sizeof(Node_hbm));
        convertNodeToRaw(node, localPage[node.idx]);
        convertNodeToRaw(newNode, localPage[newNode.idx]);
        convertNodeToRaw(newSibbling, localPage[newSibbling.idx]);
    }
    write_page(localPage, p, pageOut);
    }
}


void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, const int freeNodeIdx)
{
    if(currentNode.idx == 0){
        //New root node
        currentNode.idx = freeNodeIdx;
        newNode.idx = 0;

    }else{
        //Add node to array
        newNode.idx = freeNodeIdx;
    }
}