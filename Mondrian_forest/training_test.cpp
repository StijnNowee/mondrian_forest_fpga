 // Include common definitions and header files
// Include the top-level function implementation
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "common.hpp"
#include "train.hpp"
#include <array>
#include <fstream>
#include <string>
using namespace rapidjson;

void top_lvl(
    hls::stream<input_t> &inputFeatureStream,
    hls::stream<int> &outputStream,
    Page *pageBank1
);

void import_nodes_from_json(const std::string &filename, Page *pageBank);
void import_input_data(const std::string &filename, hls::stream<input_vector> &inputStream);
void import_input_csv(const std::string &filename, hls::stream<input_vector> &inputStream);

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
    os << "\n  labelCount: " << node.labelCount;
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
    hls::stream<input_t, 2> inputStream ("inputStream");
    hls::stream<int> outputStream ("OutputStream");

    Page pageBank1[MAX_PAGES_PER_TREE*TREES_PER_BANK];
    
    Node_hbm emptynode;
    node_t raw_emptyNode;
    memcpy(&raw_emptyNode, &emptynode, sizeof(Node_hbm));
    for(int p = 0; p < MAX_PAGES_PER_TREE*TREES_PER_BANK; p++){
        for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
            pageBank1[p][n] = raw_emptyNode;
        }
    }

    import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank1);
    //import_input_data("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/input_larger.json", inputStream);
    Node_hbm node;
    input_t rawinput;
    rawinput.range(7,0) = unit_interval(0.1).range(7,0);
    rawinput.range(15,8) = unit_interval(0.5).range(7,0);
    rawinput.range(23,16) = unit_interval(0.5).range(7,0);
    rawinput.range(31,24) = unit_interval(0.8).range(7,0);
    rawinput.range(39,32) = unit_interval(0.6).range(7,0);
    rawinput.range(71,40) = int(1);

    inputStream.write(rawinput);
    inputStream.write(rawinput);

    const int N = inputStream.size();
    top_lvl(inputStream, outputStream, pageBank1);

    int total = 0;
    for(int i = 0; i < TREES_PER_BANK*BANK_COUNT*N; i++){
        total += outputStream.read();
    }
    std::cout << "Total: " << total << std::endl;
    
    std::cout << "done"  << std::endl;
    for(int t = 0; t < TREES_PER_BANK; t++){
        for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
            for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
                convertRawToNode(pageBank1[t*MAX_PAGES_PER_TREE + p][n], node);
                if(node.valid){
                    //if(p == 0 && t==0){
                    std::cout <<"Tree: " << t << std::endl << "Page idx: " << p << std::endl << "Node idx: " << n << std::endl << node << std::endl;
                }
            }
        }
    }
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

        Node_hbm node;
        node.idx = nodeObj["idx"].GetInt();
        node.leaf = nodeObj["leaf"].GetBool();
        node.valid = nodeObj["valid"].GetBool();
        node.feature = nodeObj["feature"].GetInt();
        node.threshold = nodeObj["threshold"].GetFloat();
        node.splittime = nodeObj["splittime"].GetFloat();
        node.parentSplitTime = nodeObj["parentSplitTime"].GetFloat();
        
        // Extract arrays
        const auto& lowerBoundArr = nodeObj["lowerBound"].GetArray();
        for (SizeType i = 0; i < lowerBoundArr.Size(); i++) {
            node.lowerBound[i] = lowerBoundArr[i].GetFloat();
        }
        const auto& upperBoundArr = nodeObj["upperBound"].GetArray();
        for (SizeType i = 0; i < upperBoundArr.Size(); i++) {
            node.upperBound[i] = upperBoundArr[i].GetFloat();
        }
        const auto& classDistArr = nodeObj["classDistribution"].GetArray();
        for (SizeType i = 0; i < classDistArr.Size(); i++) {
            node.classDistribution[i] = classDistArr[i].GetInt();
        }

        // Extract child nodes
        if(!node.leaf){
            node.leftChild.id = nodeObj["leftChild"]["id"].GetInt();
            node.leftChild.isPage = nodeObj["leftChild"]["isPage"].GetBool();
            node.rightChild.id = nodeObj["rightChild"]["id"].GetInt();
            node.rightChild.isPage = nodeObj["rightChild"]["isPage"].GetBool();
        }

        //Store identical to each tree
        for(int t = 0; t < TREES_PER_BANK; t++){
            convertNodeToRaw(node, pageBank[t*MAX_PAGES_PER_TREE][node.idx]);
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

void import_input_csv(const std::string &filename, hls::stream<input_vector> &inputStream)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        return;
    }
    std::string line;
    while( std::getline(file, line)){
        std::stringstream ss(line);
        std::string value;
        input_vector input;
        for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
            std::getline(ss, value, ',');
            input.feature[i] = std::stof(value);
        }
        std::getline(ss, value, ',');
        input.label = std::stoi(value);
        inputStream.write(input);

    }
}