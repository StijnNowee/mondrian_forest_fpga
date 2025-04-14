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
    hls::stream<input_vector> inputStream[2],
    hls::stream<Result> &resultOutputStream,
    const InputSizes &sizes,
    Page* trainHBM,
    Page* inferenceHBM
);

void import_csv(const std::string &filename, hls::stream<input_vector> inputStream[2], PageBank hbmMemory[BANK_COUNT], std::vector<int> &referenceLabels);


void visualizeTree(const std::string& filename, Page *pageBank);
void generateDotFileRecursive(std::ofstream& dotFile, int currentPageIndex, int currentNodeIndex, Page *pageBank);
void construct_root_node(Node_hbm &rootNode, input_vector &firstSample);
void print_tree(const Page *pageBank, const int &treeID);

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
    hls::stream<input_vector> inputStream[2];
    hls::stream<Result> inferenceOutputStream("InferenceOutputStream");

    PageBank hbmMemory[BANK_COUNT];
    std::vector<int> referenceLabels;
    InputSizes sizes;

    import_csv("C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/syntetic_dataset_normalized.csv", inputStream, hbmMemory, referenceLabels);

    sizes.seperate[TRAIN] = 1;
    sizes.seperate[INF] = 1;
    sizes.total =2;
    int size = inputStream[TRAIN].size();    
    for(int i =0; i < size; i++){
        top_lvl(inputStream, inferenceOutputStream, sizes, (Page*)hbmMemory, (Page*)hbmMemory);
    }

    //import_inference_csv("C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/syntetic_dataset_normalized.csv", inputStream[INF]);

    // int nrOfIter = ceil(float(inputStream[TRAIN].size())/BLOCK_SIZE);
    // //First train the model
    // for(int i = 0; i < nrOfIter; i++){
    //     sizes.seperate[TRAIN] = std::min(BLOCK_SIZE, inputStream[TRAIN].size());
    //     sizes.total = sizes.seperate[TRAIN];
    //     top_lvl(inputStream, inferenceOutputStream, sizes, hbmMemory, hbmMemory);
    // }

    // nrOfIter = ceil(float(inputStream[INF].size())/BLOCK_SIZE);
    // //Then use inference
    // for(int i = 0; i < nrOfIter; i++){
    //     sizes.seperate[TRAIN] = 0;
    //     sizes.seperate[INF] = std::min(BLOCK_SIZE, inputStream[INF].size());
    //     sizes.total = sizes.seperate[INF];
    //     top_lvl(inputStream, inferenceOutputStream, sizes, hbmMemory, hbmMemory);
    // }
    int totalCorrect = 0;
    for(int i =0; i < size; i++){
        auto result = inferenceOutputStream.read();
        std::cout << "Class: " << result.resultClass << " with confidence: " << result.confidence.to_float() << std::endl;
        if(referenceLabels.at(i) == result.resultClass) totalCorrect++;
    }

    std::cout << "Total correct: " << totalCorrect << " Out of : " << size << " Accuracy: " << float(totalCorrect)/size*100.0 << std::endl;

    visualizeTree("C:/Users/stijn/Documents/Uni/Thesis/M/Tree_results/newOutput", hbmMemory[0]);

    std::cout << "done"  << std::endl;

    return 0;
}



void import_csv(const std::string &filename, hls::stream<input_vector> inputStream[2], PageBank hbmMemory[BANK_COUNT], std::vector<int> &referenceLabels)
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
            referenceLabels.push_back(input.label);
            inputStream[TRAIN].write(input);
            inputStream[INF].write(input);
        }
        
    }
}


void generateDotFileRecursive(std::ofstream& dotFile, int currentPageIndex, int currentNodeIndex, Page *pageBank) {
    
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

void print_tree(const Page *pageBank, const int &treeID)
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