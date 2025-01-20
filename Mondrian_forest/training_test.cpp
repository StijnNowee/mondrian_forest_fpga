 // Include common definitions and header files
// Include the top-level function implementation
#include <cwchar>
#include <ostream>
#include "common.hpp"

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page *pageBank1,
    hls::stream<unit_interval, 100> &rngStream1,
    hls::stream<unit_interval, 100> &rngStream2,
    int size
);

std::ostream &operator <<(std::ostream &os, const ChildNode &node){
    if(node.isPage){
       // os << "Page idx: " << node.pageIdx;
    }else{
        os << "Node idx: " << node.nodeIdx;
    }
    return os;
}

std::ostream &operator <<(std::ostream &os, const Node_hbm &node){
    os << "Node {";
    os << "\n  idx: " << node.idx;
    os << "\n  leaf: " << std::boolalpha << node.leaf;
    os << "\n  feature: " << static_cast<int>(node.feature);
    os << "\n  threshold: " << node.threshold;

    os << "\n  lowerBound: [";
        for (int i = 0; i < FEATURE_COUNT_TOTAL; ++i) {
            os << node.lowerBound[i] << (i < FEATURE_COUNT_TOTAL - 1 ? ", " : "");
        }
        os << "]";
    os << "\n  upperBound: [";
        for (int i = 0; i < FEATURE_COUNT_TOTAL; ++i) {
            os << node.upperBound[i] << (i < FEATURE_COUNT_TOTAL - 1 ? ", " : "");
        }
    os << "]";

    os << "\n  splittime: " << std::fixed << std::setprecision(6) << node.splittime;
    os << "\n  classDistribution: [";
        for (int i = 0; i < CLASS_COUNT; ++i) {
            os << node.classDistribution[i] << (i < CLASS_COUNT - 1 ? ", " : "");
        }
    os << "]";
    os << "\n  leftChild: " << node.leftChild;
    os << "\n  rightChild: " << node.rightChild;
    os << "\n  parentSplitTime: " << std::fixed << std::setprecision(6) << node.parentSplitTime;

    os << "\n}";
    return os;
}

int main() {
    // Set up streams

    hls::stream<input_vector> inputstream ("inputStream");
    hls::stream<unit_interval, 100> rngStream1 ("rngstream1");
    hls::stream<unit_interval, 100> rngStream2 ("rngstream2");

    Page pageBank1[MAX_PAGES_PER_TREE*TREES_PER_BANK];

    Node_hbm emptynode;
    node_t raw_emptyNode;
    memcpy(&raw_emptyNode, &emptynode, sizeof(Node_hbm));

    for(int p = 0; p < MAX_PAGES_PER_TREE*TREES_PER_BANK; p++){
        for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
            pageBank1[p][n] = raw_emptyNode;
        }
    }

    // for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
    //         Node_hbm node;
    //         memcpy(&node, &pageBank1[0][i], sizeof(Node_hbm));
    //         if(node.valid){
    //             std::cout <<"At index: " << i << std::endl << node << std::endl;
    //         }
    // }

    Node_hbm node, leftChild, rightChild;
    leftChild.idx = 1;
    leftChild.parentSplitTime = 2.42;
    leftChild.leaf = true;
    leftChild.feature = 10;
    leftChild.splittime = std::numeric_limits<float>::max();
    leftChild.valid = true;
    leftChild.lowerBound[0] = 0.1;
    leftChild.lowerBound[1] = 0.1;
    leftChild.upperBound[0] = 0.1;
    leftChild.upperBound[1] = 0.1;
    ChildNode left;
    left.nodeIdx = leftChild.idx;

    rightChild.idx = 2;
    rightChild.parentSplitTime = 2.42;
    rightChild.leaf = true;
    rightChild.feature = 20;
    rightChild.splittime = std::numeric_limits<float>::max();
    rightChild.lowerBound[0] = 0.4;
    rightChild.lowerBound[1] = 0.3;
    rightChild.upperBound[0] = 0.4;
    rightChild.upperBound[1] = 0.3;
    rightChild.valid = true;
    ChildNode right;
    right.nodeIdx = rightChild.idx;

    node.idx = 0;
    node.feature = 1;
    node.threshold = 0.23;
    node.splittime = 2.42;
    node.parentSplitTime = 0;
    node.leftChild = left;
    node.rightChild = right;
    node.lowerBound[0] = 0.1;
    node.lowerBound[1] = 0.1;
    node.upperBound[0] = 0.4;
    node.upperBound[1] = 0.3;
    node.valid = true;

    for(int p = 0; p < TREES_PER_BANK; p++){
        Page& page = pageBank1[p*MAX_PAGES_PER_TREE];
        memcpy(&page[node.idx], &node, sizeof(Node_hbm));
        memcpy(&page[leftChild.idx], &leftChild, sizeof(Node_hbm));
        memcpy(&page[rightChild.idx], &rightChild, sizeof(Node_hbm));
    }

    

    input_vector cFeature;
    cFeature.label = 30;
    cFeature.feature[0] = 0.9;
    cFeature.feature[1] = 0.9;
    inputstream.write(cFeature);

    input_vector dFeature;
    dFeature.label = 40;
    dFeature.feature[0] = 0.55;
    dFeature.feature[1] = 0.5;
    inputstream.write(dFeature);

    for(int i = 0; i < 50; i++){
        rngStream1.write(0.90);
        rngStream1.write(0.45);
    }
    for(int i = 0; i < 100; i++){
        rngStream2.write(0.90);
    }

    //for(int i = 0; i < 3; i++){
        top_lvl(inputstream, pageBank1, rngStream1, rngStream2, inputstream.size());
    //}

    node_converter conv;
    for(int t = 0; t < TREES_PER_BANK; t++){
        for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
            for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
                conv.raw = pageBank1[t*MAX_PAGES_PER_TREE + p][n];
                if(conv.node.valid){
                    std::cout <<"Tree: " << t << std::endl << "Page idx: " << p << std::endl << "Node idx: " << n << std::endl << conv.node << std::endl;
                }
            }
        }
    }
    // for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            
    //         conv.raw = pageBank1[0][i];
    //         if(conv.node.valid){
    //             std::cout <<"At index: " << i << std::endl << conv.node << std::endl;
    //         }
    // }

    return 0;
}
