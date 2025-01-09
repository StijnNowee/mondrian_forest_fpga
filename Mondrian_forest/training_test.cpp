 // Include common definitions and header files
#include "top_lvl.hpp" // Include the top-level function implementation

std::ostream &operator <<(std::ostream &os, const ChildNode &node){
    if(node.isPage){
        os << "Page idx: " << node.pageIdx;
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
    hls::stream<input_vector> inputstream;
    hls::stream<unit_interval, 20> rngStream1;
    hls::stream<unit_interval, 20> rngStream2;

    Page pageBank1[MAX_PAGES];


    // Initialize node banks with some dummy values
    // for (int i = 0; i < MAX_NODES_PER_PAGE; ++i) {
    //     pageBank1[0][i].idx = i;
    //     pageBank1[0][i].leftChild.nodeIdx = i+1;
    //     pageBank1[0][i].rightChild.nodeIdx = i+1;
    // }

    Node_hbm node, leftChild, rightChild;
    leftChild.idx = 1;
    leftChild.parentSplitTime = 2.42;
    leftChild.leaf = true;
    leftChild.feature = 10;
    leftChild.splittime = std::numeric_limits<float>::max();
    leftChild.valid = true;
    ChildNode left;
    left.nodeIdx = leftChild.idx;

    rightChild.idx = 2;
    rightChild.parentSplitTime = 2.42;
    rightChild.leaf = true;
    rightChild.feature = 20;
    rightChild.splittime = std::numeric_limits<float>::max();
    rightChild.valid = true;
    ChildNode right;
    right.nodeIdx = rightChild.idx;

    node.idx = 0;
    node.feature = 2;
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
    
    pageBank1[0][node.idx] = node;
    pageBank1[0][leftChild.idx] = leftChild;
    pageBank1[0][rightChild.idx] = rightChild;

    std::cout << "Before: " << std::endl;
    for(auto node : pageBank1[0]){
        if(node.valid){
            std::cout << node << std::endl;
        }
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

    for(int i = 0; i < 15; i++){
        rngStream1.write(0.90);
        rngStream2.write(0.90);
    }

    

    for(int i = 0; i < 5; i++){
        top_lvl(inputstream, pageBank1, rngStream1, rngStream2);
        for(auto node : pageBank1[0]){
            if(node.valid){
            std::cout << node << std::endl;
            }
        }
    }
    // std::cout << "After" << std::endl;
    // for(auto node : pageBank1[0]){
    //     std::cout << node << std::endl;
    // }
    
    
    
    return 0;
}
