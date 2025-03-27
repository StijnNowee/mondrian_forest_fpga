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
#include <string>
using namespace rapidjson;

void top_lvl(
    hls::stream<input_t> &inputStream,
    hls::stream<Result> &inferenceOutputStream,
    const InputSizes &sizes,
    PageBank pageBank1//, PageBank pageBank2, PageBank pageBank3, PageBank pageBank4, PageBank pageBank5, PageBank pageBank6, PageBank pageBank7, PageBank pageBank8,PageBank pageBank9, PageBank pageBank10,
    //PageBank pageBank11, PageBank pageBank12, PageBank pageBank13, PageBank pageBank14, PageBank pageBank15, PageBank pageBank16, PageBank pageBank17, PageBank pageBank18,PageBank pageBank19, PageBank pageBank20
);

void import_nodes_from_json(const std::string &filename, Page *pageBank);
void import_training_data(const std::string &filename, hls::stream<input_t> &inputStream);
void import_training_csv(const std::string &filename, hls::stream<input_t> &inputStream);
void import_inference_data(const std::string &filename, hls::stream<input_t> &inputStream);
void import_inference_csv(const std::string &filename, hls::stream<input_t> &inputStream);

void visualizeTree(const std::string& filename, Page *pageBank);
void generateDotFileRecursive(std::ofstream& dotFile, int currentPageIndex, int currentNodeIndex, Page* pageBank);



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
    hls::stream<input_t, 27> inputStream ("trainInputStream1");
    hls::stream<Result,10> inferenceOutputStream("InferenceOutputStream1");

    PageBank pageBank1, pageBank2, pageBank3, pageBank4, pageBank5, pageBank6, pageBank7, pageBank8, pageBank9, pageBank10, pageBank11, pageBank12, pageBank13, pageBank14, pageBank15, pageBank16, pageBank17, pageBank18, pageBank19, pageBank20;
   // Page pageBank2[MAX_PAGES_PER_TREE*TREES_PER_BANK];
    
    InputSizes sizes;
    //import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_clean.json", pageBank1);
    import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank1);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank2);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank3);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank4);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank5);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank6);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank7);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank8);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank9);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank10);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank11);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank12);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank13);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank14);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank15);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank16);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank17);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank18);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank19);
    // import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank20);
    //import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_clean.json", pageBank2);
    //import_training_data("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/input_larger.json", inputStream);
    import_training_csv("C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/cov_normalized_small.csv", inputStream);
    sizes.training = inputStream.size();
    //import_inference_data("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/inference_larger.json", inputStream);
    import_inference_csv("C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/cov_normalized_small.csv", inputStream);
    sizes.total = inputStream.size();
    sizes.inference = sizes.total - sizes.training;

    top_lvl(inputStream, inferenceOutputStream ,sizes ,pageBank1);//, pageBank2, pageBank3, pageBank4, pageBank5, pageBank6, pageBank7, pageBank8, pageBank9, pageBank10, pageBank11, pageBank12, pageBank13, pageBank14, pageBank15, pageBank16, pageBank17, pageBank18, pageBank19, pageBank20);

    while(!inferenceOutputStream.empty()){
        auto result = inferenceOutputStream.read();
        std::cout << "Class: " << result.resultClass << " with confidence: " << result.confidence.to_float() << std::endl;
    }

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
        const auto& classDistArr = nodeObj["classDistribution"].GetArray();
        for (SizeType i = 0; i < classDistArr.Size(); i++) {
            node.classDistribution[i].range(7, 0) = unit_interval(classDistArr[i].GetFloat());
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

void import_training_csv(const std::string &filename, hls::stream<input_t> &inputStream)
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
        input_t rawInput = convertVectorToInput(input);
        
        inputStream.write(rawInput);
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

void generateDotFileRecursive(std::ofstream& dotFile, int currentPageIndex, int currentNodeIndex, Page* pageBank) {
    
    Node_hbm currentNode(rawToNode(pageBank[currentPageIndex][currentNodeIndex]));

    // Create a unique ID for the current node.  Use page and node index.
    std::string currentNodeId = "page" + std::to_string(currentPageIndex) + "_node" + std::to_string(currentNodeIndex);

    // Handle left child
    if (!currentNode.leaf()) {
        // Add node definition to the DOT file, using a descriptive label
        dotFile << "    " << currentNodeId << " [label=\"" << "x" << currentNode.feature+1 << " > " << currentNode.threshold.to_float() << "\"];\n";
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
        for (int i = 0; i < CLASS_COUNT; ++i) {
            dotFile << currentNode.classDistribution[i].to_float() << (i < CLASS_COUNT - 1 ? ", " : "");
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
    // std::string command = "dot -Tpng " + filename + ".dot -o " + filename + ".png";
    // int result = system(command.c_str());

    // if (result != 0) {
    //     std::cerr << "Error generating image.  Make sure Graphviz (dot) is installed and in your PATH." << std::endl;
    // } else {
    //     std::cout << "Tree visualization generated: " << filename << ".png" << std::endl;
    // }
}