 // Include common definitions and header files
// Include the top-level function implementation
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "common.hpp"
#include "train.hpp"
#include <array>
#include <fstream>
#include <ostream>
#include <string>
using namespace rapidjson;

void top_lvl(
    hls::stream<input_t> &trainInputStream,
    hls::stream<input_t>  &inferenceInputStream,
    hls::stream<Result> &inferenceOutputStream,
    const int size,
    // Page *pageBank1,
    Page *pageBank1
);

void import_nodes_from_json(const std::string &filename, Page *pageBank);
void import_input_data(const std::string &filename, hls::stream<input_t> &inputStream);
void import_input_csv(const std::string &filename, hls::stream<input_t> &inputStream);

void visualizeTree(const std::string& filename, Page *pageBank);
void generateDotFileRecursive(std::ofstream& dotFile, int currentPageIndex, int currentNodeIndex, Page* pageBank);


void convertVectorToInput(const input_vector &input, input_t &raw){
    raw = *reinterpret_cast<const input_t*>(&input);
}


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
    hls::stream<input_t, 27> trainInputStream ("trainInputStream");
    hls::stream<input_t, 3001> inferenceInputStream ("inferenceInputStream");
    hls::stream<ap_uint<72>, 200> smlNodeOutputStream("SmlNodeOutputStream");
    hls::stream<node_t, 300> dataOutputStream ("DataOutputStream");
    hls::stream<bool, 27> controlOutputStream ("ControlOutputStream");
    //hls::stream<Result> resultOutputStream("ResultOutputStream");
    hls::stream<Result,10> inferenceOutputStream("InferenceOutputStream");

    Page pageBank1[MAX_PAGES_PER_TREE*TREES_PER_BANK];
    Page localStorage[MAX_PAGES_PER_TREE*TREES_PER_BANK];
    Node_sml smallNodeBank[3*MAX_NODES_PER_PAGE];
    
    Node_hbm emptynode;
    node_t raw_emptyNode;
    memcpy(&raw_emptyNode, &emptynode, sizeof(Node_hbm));
    for(int p = 0; p < MAX_PAGES_PER_TREE*TREES_PER_BANK; p++){
        for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
            pageBank1[p][n] = raw_emptyNode;
        }
    }

    //Manual inference input:
    input_vector inferenceInput;
    inferenceInput.feature[0] = 0.5;
    inferenceInput.feature[1] = 0.6;
    inferenceInput.feature[2] = 0.4;
    inferenceInput.feature[3] = 0.3;
    inferenceInput.feature[4] = 0.7;
    inferenceInput.label = 2;
    input_t rawinfInput = 0;
    convertVectorToInput(inferenceInput, rawinfInput);
    for(int i = 0; i < 10; i++){
        inferenceInputStream.write(rawinfInput);
    }

    import_nodes_from_json("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/nodes_input_larger.json", pageBank1);
    import_input_data("C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/input_larger.json", trainInputStream);
    Node_hbm node;

    const int N = trainInputStream.size();
    std::cout << "size: " << N << std::endl;
    std::cout << "inferenceInputStream size: " << inferenceInputStream.size();
    top_lvl(trainInputStream, inferenceInputStream, inferenceOutputStream ,N ,pageBank1);

    // for(int i = 0; i < TREES_PER_BANK*BANK_COUNT*N; i++){
    //     std::cout << "Sample done: " << i << std::endl;
    //     bool test = controlOutputStream.read();
    // }

    // int ptr = 0;
    // std::cout << "size: " << smlNodeOutputStream.size();
    // while(!smlNodeOutputStream.empty()){
    //     std::cout << "Node ID: " << ptr++ << std::endl;
    //     auto rawResult = smlNodeOutputStream.read();
    //     Node_sml result;
    //     *reinterpret_cast<ap_uint<72>*>(&result) = rawResult;
    //     std::cout << "  " << result.feature << " > " << result.threshold.to_float() << std::endl;
    //     if(result.leaf){
    //         std::cout << "  Leaf with distribution: [";
    //         for (int i = 0; i < CLASS_COUNT; ++i) {
    //         std::cout << result.classDistribution[i] << (i < CLASS_COUNT - 1 ? ", " : "");
    //         }
    //         std::cout << "]\n";
    //     }else{
    //         std::cout << "  Left: " << result.leftChild << ", Right: " << result.rightChild << std::endl;
    //     }
    // }

    // while(!inferenceOutputStream.empty()){
    //     auto result = inferenceOutputStream.read();
    //     std::cout << "result: " << result << std::endl;
    // }

    // while(!dataOutputStream.empty()){

    //     //std::cout << "Page send: " << ++counter << std::endl;
    //     PageProperties p;
    //     convertRawToProperties(dataOutputStream.read(), p);
    //     //std::cout << "Properties converted" << std::endl;
    //     // std::cout << "Storing in page: " << p.treeID * MAX_PAGES_PER_TREE + p.pageIdx << std::endl;
    //     for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
    //             //std::cout << "Store node idx: " << n << "in page: " << p.treeID * MAX_PAGES_PER_TREE + p.pageIdx << std::endl;
    //             localStorage[p.treeID * MAX_PAGES_PER_TREE + p.pageIdx][n] = dataOutputStream.read();
                
    //     }
    // }
    while(!inferenceOutputStream.empty()){
        auto result = inferenceOutputStream.read();
        //std::cout << "Result: " << result << std::endl;
        //std::cout << "Final result: " << result.resultClass << " with confidence: " << result.confidence << std::endl;
        std::cout << "Result: [";
        for(int i =0; i < CLASS_COUNT; i++){
            std::cout << result.distribution[i].to_float() << ", ";
        }
        std::cout << "]" << std::endl;
    }



    // for(int i = 0; i < TREES_PER_BANK*BANK_COUNT*N;){
    //     //total += outputStream.read();
    //     total++;
    //     //std::cout << "Loop nr: " << total << std::endl;

    //     if(outputStream.read_nb(endSample)){
    //         if(endSample == 1){
    //             std::cout << "sample complete: " << i++ << std::endl;            
    //         }
    //         PageProperties p = convertProperties(outputStream.read());
    //         for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
    //             //std::cout << "Store node idx: " << n << "in page: " << p.treeID * MAX_PAGES_PER_TREE + p.pageIdx << std::endl;
    //             localStorage[p.treeID * MAX_PAGES_PER_TREE + p.pageIdx][n] = outputStream.read();
                
    //         }

    //     }
    //     if(total > 200000){
    //         i++;
    //     }
    // }
    //std::cout << "Total: " << total << std::endl;
    
    std::cout << "done"  << std::endl;
    // for(int t = 0; t < TREES_PER_BANK; t++){
    //     for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
    //         for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
    //             convertRawToNode(localStorage[t*MAX_PAGES_PER_TREE + p][n], node);
    //             if(node.valid){
    //                 std::cout <<"Tree: " << t << std::endl << "Page idx: " << p << std::endl << "Node idx: " << n << std::endl << node << std::endl;
    //             }
    //         }
    //     }
    // }
    // visualizeTree("C:/Users/stijn/Documents/Uni/Thesis/M/Tree_results/newOutput", localStorage);
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
            node.classDistribution[i].range(8, 0) = ap_ufixed<9,1>(classDistArr[i].GetFloat());
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
            //  memcpy(&pageBank[t*MAX_PAGES_PER_TREE][node.idx], &node, sizeof(Node_hbm));
            convertNodeToRaw(node, pageBank[t*MAX_PAGES_PER_TREE][node.idx]);
        }
    }
}

void import_input_data(const std::string &filename, hls::stream<input_t> &inputStream)
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
        input_t rawInput = 0;
        convertVectorToInput(input, rawInput);
        inputStream.write(rawInput);
    }
}

void import_input_csv(const std::string &filename, hls::stream<input_t> &inputStream)
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
        input_t rawInput = 0;
        convertVectorToInput(input, rawInput);
        inputStream.write(rawInput);
    }
}

void generateDotFileRecursive(std::ofstream& dotFile, int currentPageIndex, int currentNodeIndex, Page* pageBank) {
    
    Node_hbm currentNode;
    // memcpy(&currentNode, &pageBank[currentPageIndex][currentNodeIndex], sizeof(Node_hbm));
    convertRawToNode(pageBank[currentPageIndex][currentNodeIndex], currentNode);

    // Create a unique ID for the current node.  Use page and node index.
    std::string currentNodeId = "page" + std::to_string(currentPageIndex) + "_node" + std::to_string(currentNodeIndex);

    // Handle left child
    if (!currentNode.leaf) {
        // Add node definition to the DOT file, using a descriptive label
        dotFile << "    " << currentNodeId << " [label=\"" << "x" << currentNode.feature+1 << " > " << currentNode.threshold.to_float() << "\"];\n";
        int leftPageIndex = currentPageIndex;
        int leftNodeIndex = currentNode.leftChild.id;
        if (currentNode.leftChild.isPage)
        {
            leftPageIndex = currentNode.leftChild.id;
            leftNodeIndex = 0;
        }
        
        std::string leftChildId = "page" + std::to_string(leftPageIndex) + "_node" + std::to_string(leftNodeIndex);
        dotFile << "    " << currentNodeId << " -> " << leftChildId  << "[label=\"Left\"];\n";
        generateDotFileRecursive(dotFile, leftPageIndex, leftNodeIndex, pageBank);

        // Handle right child
        int rightPageIndex = currentPageIndex;
        int rightNodeIndex = currentNode.rightChild.id;
        if(currentNode.rightChild.isPage){
            rightPageIndex = currentNode.rightChild.id;
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