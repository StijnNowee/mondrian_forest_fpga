 // Include common definitions and header files
// Include the top-level function implementation
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "common.hpp"
#include <fstream>
using namespace rapidjson;

void top_lvl(
    hls::stream<input_vector> &inputFeatureStream,
    Page *pageBank1,
    int size
);

void import_nodes_from_json(const std::string &filename, Page *pageBank);
void import_input_data(const std::string &filename, hls::stream<input_vector> &inputStream);

std::ostream &operator <<(std::ostream &os, const ChildNode &node){
    if(node.isPage){
       // os << "Page idx: " << node.pageIdx;
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
    // Set up streams
    static hls::stream<input_vector> inputStream ("inputStream");

    Page pageBank1[MAX_PAGES_PER_TREE*TREES_PER_BANK];

    Node_hbm emptynode;
    node_t raw_emptyNode;
    memcpy(&raw_emptyNode, &emptynode, sizeof(Node_hbm));
    for(int p = 0; p < MAX_PAGES_PER_TREE*TREES_PER_BANK; p++){
        for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
            pageBank1[p][n] = raw_emptyNode;
        }
    }

    import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input.json", pageBank1);
    import_input_data("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/input.json", inputStream);

    //for(int i = 0; i < 2; i++){
        top_lvl(inputStream, pageBank1, inputStream.size());
        std::cout << "fe: " << std::endl;
    //}
    // std::cout << "done"  << std::endl;
    // node_converter conv;
    // for(int t = 0; t < TREES_PER_BANK; t++){
    //     for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
    //         for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
    //             conv.raw = pageBank1[t*MAX_PAGES_PER_TREE + p][n];
    //             if(conv.node.valid){
    //                 std::cout <<"Tree: " << t << std::endl << "Page idx: " << p << std::endl << "Node idx: " << n << std::endl << conv.node << std::endl;
    //             }
    //         }
    //     }
    // }

    return 0;
}

void import_nodes_from_json(const std::string &filename, Page *pageBank)
{
    std::ifstream ifs(filename);
    IStreamWrapper isw(ifs);
    Document doc;
    doc.ParseStream(isw);

    for(const auto &nodeVal : doc.GetArray()){
        const Value &nodeObj = nodeVal.GetObject();

        node_converter conv;
        conv.node.idx = nodeObj["idx"].GetInt();
        conv.node.leaf = nodeObj["leaf"].GetBool();
        conv.node.valid = nodeObj["valid"].GetBool();
        conv.node.feature = nodeObj["feature"].GetInt();
        conv.node.threshold = nodeObj["threshold"].GetFloat();
        conv.node.splittime = nodeObj["splittime"].GetFloat();
        conv.node.parentSplitTime = nodeObj["parentSplitTime"].GetFloat();
        
        // Extract arrays
        const auto& lowerBoundArr = nodeObj["lowerBound"].GetArray();
        for (SizeType i = 0; i < lowerBoundArr.Size(); i++) {
            conv.node.lowerBound[i] = lowerBoundArr[i].GetFloat();
        }
        const auto& upperBoundArr = nodeObj["upperBound"].GetArray();
        for (SizeType i = 0; i < upperBoundArr.Size(); i++) {
            conv.node.upperBound[i] = upperBoundArr[i].GetFloat();
        }
        const auto& classDistArr = nodeObj["classDistribution"].GetArray();
        for (SizeType i = 0; i < classDistArr.Size(); i++) {
            conv.node.classDistribution[i] = classDistArr[i].GetInt();
        }

        // Extract child nodes
        if(!conv.node.leaf){
            conv.node.leftChild.id = nodeObj["leftChild"]["id"].GetInt();
            conv.node.leftChild.isPage = nodeObj["leftChild"]["isPage"].GetBool();
            conv.node.rightChild.id = nodeObj["rightChild"]["id"].GetInt();
            conv.node.rightChild.isPage = nodeObj["rightChild"]["isPage"].GetBool();
        }

        //Store identical to each tree
        for(int t = 0; t < TREES_PER_BANK; t++){
            pageBank[t*MAX_PAGES_PER_TREE][conv.node.idx] = conv.raw;
        }
    }
}

void import_input_data(const std::string &filename, hls::stream<input_vector> &inputStream)
{
    std::ifstream ifs(filename);
    IStreamWrapper isw(ifs);
    Document doc;
    doc.ParseStream(isw);
    for(const auto &inputVal : doc.GetArray()){
        const Value &inputObj = inputVal.GetObject();
        input_vector input;
        input.label = inputObj["label"].GetInt();
        const auto& featureArr = inputObj["feature"].GetArray();
        for(SizeType i = 0; i < featureArr.Size(); i++){
            input.feature[i] = featureArr[i].GetFloat();
        }
        inputStream.write(input);
    }
}