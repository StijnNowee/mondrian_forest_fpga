#include "train.hpp"

void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, const int freeNodeIdx);

void node_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut)
{
        //Copy input
        hls::read_lock<IPage> in(pageIn);
        hls::write_lock<IPage> out(pageOut);
        save_to_output: for(int i = 0; i < MAX_NODES_PER_PAGE + 1; i++){
            #pragma HLS UNROLL
            out[i] = in[i];
        }

        PageProperties p = convertProperties(out[MAX_NODES_PER_PAGE]);

        if(p.split.enabled){

            Node_hbm node;
            convertRawToNode(out[p.split.nodeIdx], node);

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

            if(node.idx != 0){
                Node_hbm parent;
                convertRawToNode(out[p.split.parentIdx], parent);

                //Update connections of other nodes
                if(parent.leftChild.id == node.idx){
                    parent.leftChild.id = newNode.idx;
                }else{
                    parent.rightChild.id = newNode.idx;
                }
                convertNodeToRaw(parent, out[parent.idx]);
            }

            //Write new node
            convertNodeToRaw(node, out[node.idx]);
            convertNodeToRaw(newNode, out[newNode.idx]);
            convertNodeToRaw(newSibbling, out[newSibbling.idx]);
        }
        out[MAX_NODES_PER_PAGE] = convertProperties(p);
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