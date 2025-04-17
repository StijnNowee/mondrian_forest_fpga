 // Include common definitions and header files
// Include the top-level function implementation
#include "common.hpp"
#include "converters.hpp"
#include <array>
#include <fstream>
#include <ostream>
#include <iostream>
#include <string>
#include <hls_stream.h>

void top_lvl(
    hls::stream<input_vector> inputStream[2],
    hls::stream<Result> &resultOutputStream,
    hls::stream<int> executionCountStream[BANK_COUNT],
    int sizes[2],
    PageBank trainHBM[BANK_COUNT],
    PageBank inferenceHBM[BANK_COUNT]
);

void import_csv(const std::string &filename, hls::stream<input_vector> inputStream[2], PageBank hbmMemory[BANK_COUNT], std::vector<int> &referenceLabels);

long countFileLines(const std::string& filename);
int prepare_next_input(hls::stream<input_vector> inputStream[2], std::ifstream &file);
void process_output(hls::stream<Result> &inferenceOutputStream, hls::stream<int> executionCountStream[BANK_COUNT], const int &correctLabel, int &totalCorrect, int &totalExecutions);
void visualizeTree(const std::string& filename, Page *pageBank);
void generateDotFileRecursive(std::ofstream& dotFile, int currentPageIndex, int currentNodeIndex, Page *pageBank);
void construct_root_node(Node_hbm &rootNode, std::ifstream &file);


int main() {
    // Set up streams
    hls::stream<input_vector> inputStream[2];
    hls::stream<Result> inferenceOutputStream("InferenceOutputStream");
    hls::stream<int> executionCountStream[BANK_COUNT];

    PageBank hbmMemory[BANK_COUNT];
    std::string datasetLocation;
    //Inference, Training
    int sizes[] = {1, 1};

    #ifdef SYN
    datasetLocation = "C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/Normalized/syntetic_dataset_normalized.csv";
    #elifdef ARG
    datasetLocation = "C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/Original/agrawal_2_w50k/2.csv";
    #endif

    #ifdef CALIBRATION
    long lineCount = COSIM_SAMPLE_SIZE;
    #else
    long lineCount = countFileLines(datasetLocation); 
    #endif

    std::ifstream file(datasetLocation);
    Node_hbm rootNode;
    construct_root_node(rootNode, file);
    for(int b = 0; b < BANK_COUNT; b++){
        for(int t = 0; t < TREES_PER_BANK; t++){
            hbmMemory[b][t*MAX_PAGES_PER_TREE][0] = nodeToRaw(rootNode);
        }
    }
    lineCount--;
    

    int totalCorrect = 0, totalExecutions = 0;
    for(int i = 0; i < lineCount; i++){
        std::cout << "Sample: " << i << std::endl;
        int correctLabel = prepare_next_input(inputStream, file);
        top_lvl(inputStream, inferenceOutputStream, executionCountStream ,sizes, hbmMemory, hbmMemory);
        process_output(inferenceOutputStream, executionCountStream, correctLabel, totalCorrect, totalExecutions);
    }


    std::cout << "Total correct: " << totalCorrect << " Out of : " << lineCount << " Accuracy: " << float(totalCorrect)/lineCount*100.0 << std::endl;
    #ifndef IMPLEMENTING
    std::cout << "Total executions: " << totalExecutions << std::endl;
    #endif

    return 0;
}

int prepare_next_input(hls::stream<input_vector> inputStream[2], std::ifstream &file)
{
    std::string line;
    input_vector newInput;
    std::getline(file, line);
    std::stringstream ss(line);
    std::string value;

    for (int i = 0; i < FEATURE_COUNT_TOTAL; ++i) {
        std::getline(ss, value, ',');
        newInput.feature[i] = std::stof(value);
    }

    std::getline(ss, value, ',');
    newInput.label = std::stoi(value);
    inputStream[0].write(newInput);
    inputStream[1].write(newInput);
    return newInput.label;

}

void process_output(hls::stream<Result> &inferenceOutputStream, hls::stream<int> executionCountStream[BANK_COUNT], const int &correctLabel, int &totalCorrect, int &totalExecutions)
{
    auto result = inferenceOutputStream.read();
    if(result.resultClass == correctLabel){
        totalCorrect++;
    }
    #ifndef IMPLEMENTING
    int maxExecutionCount = 0;
    for(int b = 0; b < BANK_COUNT; b++){
        int count = executionCountStream[b].read();
        if(count > maxExecutionCount){
            maxExecutionCount = count;
        }
    }
    totalExecutions += maxExecutionCount;
    #endif
}



// void import_csv(const std::string &filename, hls::stream<input_vector> inputStream[2], PageBank hbmMemory[BANK_COUNT], std::vector<int> &referenceLabels)
// {
//     std::ifstream file(filename);
//     if (!file.is_open()) {
//         std::cerr << "Error: Could not open file: " << filename << std::endl;
//         return;
//     }
//     std::string line;
//     bool firstSample = true;
//     while( std::getline(file, line)){
//         std::stringstream ss(line);
//         std::string value;
//         input_vector input;
//         for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
//             std::getline(ss, value, ',');
//             input.feature[i] = std::stof(value);
//         }
//         std::getline(ss, value, ',');
//         input.label = std::stoi(value);
        
//         if(firstSample){
//             firstSample = false;
//             Node_hbm rootNode;
//             construct_root_node(rootNode, input);
//             for(int b = 0; b < BANK_COUNT; b++){
//                 for(int t = 0; t < TREES_PER_BANK; t++){
//                     hbmMemory[b][t*MAX_PAGES_PER_TREE][0] = nodeToRaw(rootNode);
//                 }
//             }
//         }else{
//             referenceLabels.push_back(input.label);
//             inputStream[TRAIN].write(input);
//             inputStream[INF].write(input);
//         }
        
//     }
// }

bool readNextSample(std::ifstream& file, input_vector& sample, int& referenceLabel) {
    std::string line;
    if (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string value;

        for (int i = 0; i < FEATURE_COUNT_TOTAL; ++i) {
            std::getline(ss, value, ',');
            sample.feature[i] = std::stof(value);
        }

        std::getline(ss, value, ',');
        sample.label = std::stoi(value);
        referenceLabel = sample.label;

        return true;
    } else {
        return false;
    }
}


long countFileLines(const std::string& filename) {
    std::ifstream inputFile(filename); 

    long lineCount = 0;
    std::string line;

    while (std::getline(inputFile, line)) {
        lineCount++;
    }
    return lineCount;
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

void construct_root_node(Node_hbm &rootNode, std::ifstream &file)
{
    std::string line;
    input_vector firstSample;
    std::getline(file, line);
    std::stringstream ss(line);
    std::string value;

    for (int i = 0; i < FEATURE_COUNT_TOTAL; ++i) {
        std::getline(ss, value, ',');
        firstSample.feature[i] = std::stof(value);
    }

    std::getline(ss, value, ',');
    firstSample.label = std::stoi(value);

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