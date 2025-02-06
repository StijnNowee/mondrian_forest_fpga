#include "common.hpp"
#include "train.hpp"

std::ostream &operator <<(std::ostream &os, const ChildNode &node){
    if(node.isPage){
        os << "Page idx: " << node.id;
    }else{
        os << "Node idx: " << node.id;
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
    Node_hbm initalNode;

    initalNode.idx = 15;
    initalNode.leaf = true;
    unit_interval distr[4] ={0.1, 0.7, 0.4 ,0.23};
    for(int i = 0; i < CLASS_COUNT; i++){
        initalNode.classDistribution[i] = distr[i];
    }
    initalNode.feature = 50;
    initalNode.leftChild.id = 100;
    initalNode.leftChild.isPage  = true;
    initalNode.rightChild.id = 654;
    initalNode.rightChild.isPage = true;
    initalNode.lowerBound[0] = 0.33;
    initalNode.lowerBound[1] = 0.88;
    initalNode.upperBound[0] = 0.56;
    initalNode.upperBound[1] = 0.9;
    initalNode.parentSplitTime = 24.34;
    initalNode.splittime = 54.2;
    initalNode.threshold = 0.7;
    initalNode.valid = true;

    node_t initraw = convertNode(initalNode);
    Node_hbm newNode = convertNode(initraw);
    std::cout << "Before conversion: " << initalNode << std::endl;
    std::cout << "After conversion: " << newNode << std::endl;

    PageProperties p;
    p.nextPageIdx = 43;
    p.pageIdx = 546;
    p.dontIterate = true;
    p.freeNodesIdx[0] = 43;
    p.freeNodesIdx[1] = 54;
    p.input.feature[0] = 0.4;
    p.input.feature[1] = 0.9;
    p.input.label = 34;
    p.treeID = 376;
    p.split.enabled = true;
    p.split.newSplitTime = 56.34;
    p.split.dimension = 6;
    p.split.nodeIdx = 456;
    p.split.parentIdx = 1209;

    node_t intpraw = convertProperties(p);
    PageProperties newP = convertProperties(intpraw);

}