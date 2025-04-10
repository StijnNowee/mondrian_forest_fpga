 // Include common definitions and header files
// Include the top-level function implementation
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "common.hpp"
#include "rapidjson/rapidjson.h"
#include "converters.hpp"
#include <array>
#include <fstream>
#include <ostream>
#include <iostream>
#include <string>
using namespace rapidjson;

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &resultOutputStream,
    const InputSizes &sizes,
    PageBank hbmTrainMemory[BANK_COUNT],
    PageBank hbmInferenceMemory[BANK_COUNT]
);

void import_nodes_from_json(const std::string &filename, Page *pageBank);
void import_training_data(const std::string &filename, hls::stream<input_t> &inputStream);
void import_training_csv(const std::string &filename, hls::stream<input_t> &inputStream, PageBank hbmMemory[BANK_COUNT]);
void import_inference_data(const std::string &filename, hls::stream<input_t> &inputStream);
void import_inference_csv(const std::string &filename, hls::stream<input_t> &inputStream);

void visualizeTree(const std::string& filename, Page *pageBank);
void generateDotFileRecursive(std::ofstream& dotFile, int currentPageIndex, int currentNodeIndex, PageBank pageBank);
void construct_root_node(Node_hbm &rootNode, input_vector &firstSample);
void print_tree(const PageBank pageBank, const int &treeID);



std::ostream &operator <<(std::ostream &os, ChildNode &node){
    if(node.isPage()){
        os << "Page idx: " << node.id();
    }else{
        os << "Node idx: " << node.id();
    }
    return os;
}

std::ostream &operator <<(std::ostream &os, Node_hbm &node){
    os << "Node {";
    os << "\n  idx: " << node.idx();
    os << "\n  leaf: " << std::boolalpha << node.leaf();
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
    os << "]";
    os << "\n  leftChild: " << node.leftChild;
    os << "\n  rightChild: " << node.rightChild;
    os << "\n  parentSplitTime: " << std::fixed << std::setprecision(6) << node.parentSplitTime;
    os << "\n  posterior: [";
        for(int i = 0; i < CLASS_COUNT; i++){
            os << node.weight[i] << (i < CLASS_COUNT - 1 ? ", " : "");
        }
    os << "]";

    os << "\n}";
    return os;
}

int main() {
    // Set up streams
    hls::stream<input_t, 27> inputStream ("trainInputStream1");
    hls::stream<Result,10> inferenceOutputStream("InferenceOutputStream1");

    PageBank hbmMemory[BANK_COUNT];
    
    InputSizes sizes;
    // for(int b = 0; b < BANK_COUNT; b++){
    //     import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_clean.json", hbmMemory[b]);
    // }

    import_training_csv("C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/syntetic_dataset_normalized.csv", inputStream, hbmMemory);
    
    sizes.training = inputStream.size();
    import_inference_csv("C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/syntetic_dataset_normalized.csv", inputStream);
    //import_inference_data("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/inference_larger.json", inputStream);
    //import_inference_csv("C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/cov_normalized_xs.csv", inputStream);
    sizes.total = inputStream.size();
    sizes.inference = sizes.total - sizes.training;

    top_lvl(inputStream, inferenceOutputStream, sizes, hbmMemory, hbmMemory);

    while(!inferenceOutputStream.empty()){
        auto result = inferenceOutputStream.read();
        std::cout << "Class: " << result.resultClass << " with confidence: " << result.confidence.to_float() << std::endl;
    }

    visualizeTree("C:/Users/stijn/Documents/Uni/Thesis/M/Tree_results/newOutput", hbmMemory[0]);

    std::cout << "done"  << std::endl;

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
        node.idx(nodeObj["idx"].GetInt());
        node.leaf(nodeObj["leaf"].GetBool());
        node.valid(nodeObj["valid"].GetBool());
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

        // Extract child nodes
        if(!node.leaf()){
            node.leftChild.id(nodeObj["leftChild"]["id"].GetInt());
            node.leftChild.isPage(nodeObj["leftChild"]["isPage"].GetBool());
            node.rightChild.id(nodeObj["rightChild"]["id"].GetInt());
            node.rightChild.isPage(nodeObj["rightChild"]["isPage"].GetBool());
        }

        //Store identical to each tree
        for(int t = 0; t < TREES_PER_BANK; t++){
            pageBank[t*MAX_PAGES_PER_TREE][node.idx()] = nodeToRaw(node);
        }
    }
}

void import_training_data(const std::string &filename, hls::stream<input_t> &inputStream)
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
        input_t rawInput = convertVectorToInput(input);
        inputStream.write(rawInput);
    }
}

void import_inference_data(const std::string &filename, hls::stream<input_t> &inputStream)
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
        input.inferenceSample = true;
        input_t rawInput = convertVectorToInput(input);
        
        inputStream.write(rawInput);
    }
}

void import_training_csv(const std::string &filename, hls::stream<input_t> &inputStream, PageBank hbmMemory[BANK_COUNT])
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        return;
    }
    std::string line;
    bool firstSample = true;
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
        input_t rawInput = convertVectorToInput(input);
        if(firstSample){
            firstSample = false;
            Node_hbm rootNode;
            construct_root_node(rootNode, input);
            for(int b = 0; b < BANK_COUNT; b++){
                for(int t = 0; t < TREES_PER_BANK; t++){
                    hbmMemory[b][t*MAX_PAGES_PER_TREE][0] = nodeToRaw(rootNode);
                }
            }
        }else{
            inputStream.write(rawInput);
        }
        
    }
}

void import_inference_csv(const std::string &filename, hls::stream<input_t> &inputStream)
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
        input.inferenceSample = true;
        input_t rawInput = convertVectorToInput(input);
        inputStream.write(rawInput);
    }
}

void generateDotFileRecursive(std::ofstream& dotFile, int currentPageIndex, int currentNodeIndex, PageBank pageBank) {
    
    Node_hbm currentNode(rawToNode(pageBank[currentPageIndex][currentNodeIndex]));

    // Create a unique ID for the current node.  Use page and node index.
    std::string currentNodeId = "page" + std::to_string(currentPageIndex) + "_node" + std::to_string(currentNodeIndex);

    // Handle left child
    if (!currentNode.leaf()) {
        // Add node definition to the DOT file, using a descriptive label
        dotFile << "    " << currentNodeId << " [label=\"" << "x" << currentNode.feature+1 << " > " << currentNode.threshold.to_float() << "\n";
        for(int c = 0; c < CLASS_COUNT; c++){
            dotFile << currentNode.weight[c].to_float() << (c < CLASS_COUNT - 1 ? ", " : "");
        }
        dotFile << "\"];\n";
        int leftPageIndex = currentPageIndex;
        int leftNodeIndex = currentNode.leftChild.id();
        if (currentNode.leftChild.isPage())
        {
            leftPageIndex = currentNode.leftChild.id();
            leftNodeIndex = 0;
        }
        
        std::string leftChildId = "page" + std::to_string(leftPageIndex) + "_node" + std::to_string(leftNodeIndex);
        dotFile << "    " << currentNodeId << " -> " << leftChildId  << "[label=\"Left\"];\n";
        generateDotFileRecursive(dotFile, leftPageIndex, leftNodeIndex, pageBank);

        // Handle right child
        int rightPageIndex = currentPageIndex;
        int rightNodeIndex = currentNode.rightChild.id();
        if(currentNode.rightChild.isPage()){
            rightPageIndex = currentNode.rightChild.id();
            rightNodeIndex = 0;
        }

        std::string rightChildId = "page" + std::to_string(rightPageIndex) + "_node" + std::to_string(rightNodeIndex);
        dotFile << "    " << currentNodeId << " -> " << rightChildId << "[label=\"Right\"];\n";
        generateDotFileRecursive(dotFile, rightPageIndex, rightNodeIndex, pageBank);
    }else{
        dotFile << "    " << currentNodeId << " [label=\"";
        for(int c = 0; c < CLASS_COUNT; c++){
            dotFile << currentNode.weight[c].to_float() << (c < CLASS_COUNT - 1 ? ", " : "");
        }
        dotFile << "\"];\n";
    }
}

void visualizeTree(const std::string& filename, Page *pageBank) {
    std::ofstream dotFile(filename + ".dot");
    if (!dotFile.is_open()) {
        std::cerr << "Error opening file for writing!" << std::endl;
        return;
    }

    dotFile << "digraph Tree {\n";
    dotFile << "node [shape=box];\n"; // Make nodes rectangular
    generateDotFileRecursive(dotFile, 0, 0, pageBank); // Start at page 0, node 0
    dotFile << "}\n";
    dotFile.close();

    // Generate the image using the 'dot' command.
    //"dot -Tsvg " + filename + ".dot -o " + filename + ".svg";

}

void construct_root_node(Node_hbm &rootNode, input_vector &firstSample)
{
    rootNode.idx(0);
    rootNode.leaf(true);
    rootNode.valid(true);
    rootNode.parentSplitTime = 0;
    rootNode.splittime = MAX_LIFETIME;
    rootNode.threshold = 0;
    rootNode.feature = 0;

    for(int d = 0; d < FEATURE_COUNT_TOTAL; d++){
        rootNode.upperBound[d] = firstSample.feature[d];
        rootNode.lowerBound[d] = firstSample.feature[d];
    }
    rootNode.counts[firstSample.label] = 1;
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        rootNode.weight[c] = (rootNode.counts[c] + unit_interval(ALPHA)) /(1 + ap_ufixed<8,7>(BETA));
    }

}

void print_tree(const PageBank pageBank, const int &treeID)
{
    for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
        for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
            Node_hbm node = rawToNode(pageBank[treeID*MAX_PAGES_PER_TREE + p][n]);
            if(node.valid()){
                std::cout << node << std::endl;
            }
        }
        
    }
}