#include "train.hpp"

void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, hls::write_lock<IPage> &out, const int freeNodeIdx);

void node_splitter(hls::stream_of_blocks<IPage> &pageIn, hls::stream<unit_interval, 100> &splitterRNGStream, hls::stream_of_blocks<IPage> &pageOut, const int loopCount, hls::stream<bool> &treeDoneStream)
{
    main_loop: for(int iter = 0; iter < loopCount;){
        //Copy input
        if(!pageIn.empty()){
        std::cout << "test4: " << iter << std::endl;
        hls::read_lock<IPage> in(pageIn);
        hls::write_lock<IPage> out(pageOut);
        save_to_output: for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            out[i] = in[i];
        }

        auto p = convertRawToProperties(in[MAX_NODES_PER_PAGE]);
        std::cout << "in node splitter: " << p.treeID << std::endl;

        if(p.split.enabled){

            auto &sp = p.split;
            node_converter current(out[sp.nodeIdx]);

            auto featureValue = p.input.feature[sp.dimension]; 
            unit_interval upperBound = current.node.lowerBound[sp.dimension], lowerBound = current.node.upperBound[sp.dimension]; //Intended
            //Seems strange but saves a operation
            if(featureValue > lowerBound){
                upperBound = featureValue;
            }else{
                lowerBound = featureValue;
            }

            //Create the two new nodes
            node_converter newNode(p.split.dimension, 
                                p.split.newSplitTime, 
                                current.node.parentSplitTime,
                                lowerBound + unit_interval(0.9) * (upperBound - lowerBound), 
                                false);

            assign_node_idx(current.node, newNode.node, out, p.freeNodesIdx[0]);

            node_converter newSibbling(p.input.label, 
                                        std::numeric_limits<float>::max(),
                                        newNode.node.splittime, 
                                        0, 
                                        true, 
                                        p.freeNodesIdx[1]);
            
            //New lower and upper bounds
            for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
                #pragma HLS PIPELINE
                auto feature = p.input.feature[d];

                newNode.node.lowerBound[d] = (current.node.lowerBound[d] > feature) ? feature : current.node.lowerBound[d];
                newNode.node.upperBound[d] = (feature > current.node.upperBound[d]) ? feature : current.node.upperBound[d];
                newSibbling.node.lowerBound[d] = feature;
                newSibbling.node.upperBound[d] = feature;
            }

            if(p.input.feature[sp.dimension] <= newNode.node.threshold){
                newNode.node.leftChild = ChildNode(false, newSibbling.node.idx);
                newNode.node.rightChild = ChildNode(false, current.node.idx);
            }else{
                newNode.node.leftChild = ChildNode(false, current.node.idx);
                newNode.node.rightChild = ChildNode(false, newSibbling.node.idx);
            };
            current.node.parentSplitTime = newNode.node.splittime;

            if(current.node.idx != 0){
                node_converter parent(out[sp.parentIdx]);

                //Update connections of other nodes
                if(parent.node.leftChild.id == current.node.idx){
                    parent.node.leftChild.id = newNode.node.idx;
                }else{
                    parent.node.rightChild.id = newNode.node.idx;
                }
                out[parent.node.idx] = parent.raw;
            }

            //Write new node
            out[current.node.idx] = current.raw;
            out[newNode.node.idx] = newNode.raw;
            out[newSibbling.node.idx] = newSibbling.raw;
        }
        std::cout << "after node splitter: " << p.treeID << std::endl;
        out[MAX_NODES_PER_PAGE] = convertPropertiesToRaw(p);
        #if(not defined __SYNTHESIS__)
            if(!p.dontIterate){
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

void assign_node_idx(Node_hbm &currentNode, Node_hbm &newNode, hls::write_lock<IPage> &out, const int freeNodeIdx)
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