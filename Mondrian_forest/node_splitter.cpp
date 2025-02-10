#include "train.hpp"

void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, const int freeNodeIdx);

void node_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut, const int loopCount, hls::stream<bool> &treeDoneStream)
{
    main_loop: for(int iter = 0; iter < loopCount;){
        //Copy input
        if(!pageIn.empty()){
        hls::read_lock<IPage> in(pageIn);
        hls::write_lock<IPage> out(pageOut);
        save_to_output: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            #pragma HLS UNROLL
            out[i] = in[i];
        }

        auto p = convertProperties(in[MAX_NODES_PER_PAGE]);

        if(p.split.enabled){

            auto &sp = p.split;
            Node_hbm node = convertNode(out[sp.nodeIdx]);

            auto featureValue = p.input.feature[sp.dimension]; 
            unit_interval upperBound = node.lowerBound[sp.dimension], lowerBound = node.upperBound[sp.dimension]; //Intended
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
            
            //New lower and upper bounds
            for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                #pragma HLS PIPELINE
                auto feature = p.input.feature[d];

                newNode.lowerBound[d] = (node.lowerBound[d] > feature) ? feature : node.lowerBound[d];
                newNode.upperBound[d] = (feature > node.upperBound[d]) ? feature : node.upperBound[d];
                newSibbling.lowerBound[d] = feature;
                newSibbling.upperBound[d] = feature;
            }

            if(p.input.feature[sp.dimension] <= newNode.threshold){
                newNode.leftChild = ChildNode(false, newSibbling.idx);
                newNode.rightChild = ChildNode(false, node.idx);
            }else{
                newNode.leftChild = ChildNode(false, node.idx);
                newNode.rightChild = ChildNode(false, newSibbling.idx);
            };
            node.parentSplitTime = p.split.newSplitTime;

            if(node.idx != 0){
                Node_hbm parent = convertNode(out[sp.parentIdx]);

                //Update connections of other nodes
                if(parent.leftChild.id == node.idx){
                    parent.leftChild.id = newNode.idx;
                }else{
                    parent.rightChild.id = newNode.idx;
                }
                out[parent.idx] = convertNode(parent);
            }

            //Write new node
            out[node.idx] = convertNode(node);
            out[newNode.idx] = convertNode(newNode);
            out[newSibbling.idx] = convertNode(newSibbling);
        }
        out[MAX_NODES_PER_PAGE] = convertProperties(p);
        #if(not defined __SYNTHESIS__)
            if(!p.extraPage){
                iter++;
            }
        #endif
        }
        #if(defined __SYNTHESIS__)
             if(!treeDoneStream.empty()){
                treeDoneStream.read();
                iter++;
            }
        #endif
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