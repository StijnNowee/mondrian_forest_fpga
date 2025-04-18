 // Include common definitions and header files
// Include the top-level function implementation
#include "common.hpp"
#include "converters.hpp"
#include <array>
#include <cstdio>
#include <fstream>
#include <ostream>
#include <iostream>
#include <string>
#include <hls_stream.h>

void top_lvl(
    hls::stream<input_vector> inputStream[2],
    hls::stream<Result> &resultOutputStream,
    hls::stream<int> executionCountStream[BANK_COUNT],
    int maxPageNr[BANK_COUNT][TREES_PER_BANK],
    int sizes[2],
    PageBank trainHBM[BANK_COUNT],
    PageBank inferenceHBM[BANK_COUNT]
);

void import_csv(const std::string &filename, hls::stream<input_vector> inputStream[2], PageBank hbmMemory[BANK_COUNT], std::vector<int> &referenceLabels);

long countFileLines(const std::string& filename, const int &batchNr);
int prepare_next_input(hls::stream<input_vector> inputStream[2], std::ifstream &file);
void process_output(hls::stream<Result> &inferenceOutputStream, hls::stream<int> executionCountStream[BANK_COUNT], const int &correctLabel, int &totalCorrect, int &totalExecutions);
void visualizeTree(const std::string& filename, Page *pageBank);
void generateDotFileRecursive(std::ofstream& dotFile, int currentPageIndex, int currentNodeIndex, Page *pageBank);
void construct_root_node(Node_hbm &rootNode, std::ifstream &file);
void importMemory(const std::string& filename, PageBank* hbmMemory_ptr, int &batchNr, int &totalExecutions, int &totalCorrect, int maxPageNr[BANK_COUNT][TREES_PER_BANK]);
void exportMemory(const std::string& filename, const PageBank* hbmMemory_ptr, const int &batchNr, const int &totalExecutions, const int &totalCorrect, const int maxPageNr[BANK_COUNT][TREES_PER_BANK]);
void setStartLocation(std::ifstream &file, const int &batchNr);

#define BATCH_SIZE 5000

int main() {
    // Set up streams
    hls::stream<input_vector> inputStream[2];
    hls::stream<Result> inferenceOutputStream("InferenceOutputStream");
    hls::stream<int> executionCountStream[BANK_COUNT];

    PageBank hbmMemory[BANK_COUNT];
    std::string datasetLocation;
    #ifdef SYN
    datasetLocation = "C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/Normalized/syntetic_dataset_normalized.csv";
    #elifdef ARG
    datasetLocation = "C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/Original/agrawal_2_w50k/2.csv";
    #elifdef KDD
    datasetLocation = "C:/Users/stijn/Documents/Uni/Thesis/M/Datasets/Normalized/kddcup_log_normalized.csv";
    #endif

    //Inference, Training
    int sizes[] = {1, 1};

    int batchNr = 0, totalCorrect = 0, totalExecutions = 0;
    int maxPageNr[BANK_COUNT][TREES_PER_BANK] = {0};
    std::ifstream file(datasetLocation);



    const std::string intermediateStorage= "C:/Users/stijn/Documents/Uni/Thesis/M/Mondrian_forest/intermediate_storage/hbm_memory.bin";
    //Check if intermediate storage binary file exists
    std::ifstream checkFile(intermediateStorage, std::ios::binary);
    if (checkFile) {
        checkFile.close();
        std::cout << "Intermediate storage file found. Importing memory..." << std::endl;
        importMemory(intermediateStorage, hbmMemory, batchNr, totalExecutions, totalCorrect, maxPageNr);
    } else {
        std::cout << "No intermediate storage file found. Proceeding with training..." << std::endl;
        // Initialize HBM memory
        Node_hbm rootNode;
        construct_root_node(rootNode, file);
        for(int b = 0; b < BANK_COUNT; b++){
            for(int t = 0; t < TREES_PER_BANK; t++){
                hbmMemory[b][t*MAX_PAGES_PER_TREE][0] = nodeToRaw(rootNode);
            }
        }
    }

    #ifdef CALIBRATION
    long lineCount = COSIM_SAMPLE_SIZE;
    #else
    long lineCount = countFileLines(datasetLocation, batchNr); 
    #endif
    
    if(batchNr == 0){
        lineCount--;
    }

    setStartLocation(file, batchNr);
    
    for(int i = 0; i < lineCount; i++){
        std::cout << "Sample: " << i << std::endl;
        
        int correctLabel = prepare_next_input(inputStream, file);
        top_lvl(inputStream, inferenceOutputStream, executionCountStream, maxPageNr ,sizes, hbmMemory, hbmMemory);
        process_output(inferenceOutputStream, executionCountStream, correctLabel, totalCorrect, totalExecutions);
    }

    int totalLineCount = lineCount + batchNr*BATCH_SIZE;
    std::cout << "Total correct: " << totalCorrect << " Out of : " << totalLineCount << " Accuracy: " << float(totalCorrect)/totalLineCount*100.0 << std::endl;
    #ifndef IMPLEMENTING
    std::cout << "Total executions: " << totalExecutions << std::endl;
    #endif

    //Export memory to file
    batchNr++;
    exportMemory(intermediateStorage, hbmMemory, batchNr, totalExecutions, totalCorrect, maxPageNr);

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

void setStartLocation(std::ifstream &file, const int &batchNr){
    std::string line;
    for(int i = 0; i < batchNr*BATCH_SIZE; i++){
        std::getline(file, line);
    }   
    
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


long countFileLines(const std::string& filename, const int &batchNr) {
    std::ifstream inputFile(filename); 

    long lineCount = 0;
    std::string line;
    long currentLineIndex = 0;
 

    while (std::getline(inputFile, line)) {
        if (currentLineIndex >= batchNr*BATCH_SIZE && currentLineIndex < (batchNr+1)*BATCH_SIZE) {
            lineCount++;
        }
        currentLineIndex++;

        // Stop reading if we've passed the desired range
        if (currentLineIndex >= (batchNr+1)*BATCH_SIZE) {
            break;
        }
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
        ap_ufixed<8,0,AP_TRN, AP_SAT> tmp = std::stof(value);
        firstSample.feature[i] = tmp; 
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

void exportMemory(const std::string& filename, const PageBank* hbmMemory_ptr, const int &batchNr, const int &totalExecutions, const int &totalCorrect, const int maxPageNr[BANK_COUNT][TREES_PER_BANK]) {

    // Open file in write binary mode
    FILE* fp = fopen(filename.c_str(), "wb");

    fwrite(&batchNr, sizeof(int), 1, fp);
    fwrite(&totalExecutions, sizeof(int), 1, fp);
    fwrite(&totalCorrect, sizeof(int), 1, fp);
    fwrite(&maxPageNr, sizeof(int), BANK_COUNT*TREES_PER_BANK, fp);

    // Cast the PageBank pointer to a node_t pointer to write raw node data
    const node_t* data_ptr = reinterpret_cast<const node_t*>(hbmMemory_ptr);


    // Perform a single large write operation for maximum efficiency
    size_t items_written = fwrite(data_ptr,           // Pointer to the start of node_t data
                                  sizeof(node_t), // Size of one node_t element
                                  BANK_COUNT*TREES_PER_BANK*MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE,     // Total number of node_t elements to write
                                  fp);                // File pointer

    // --- Error Checking after Write ---
    if (fflush(fp) != 0) {
         perror(("Error flushing write buffer for file: " + filename).c_str());
         fclose(fp);
         throw std::runtime_error("Failed to flush data to file: " + filename);
    }
    if (ferror(fp)) {
         perror(("Error during writing to file: " + filename).c_str());
         fclose(fp);
         throw std::runtime_error("Error state set during file write: " + filename);
    }
    if (fclose(fp) != 0) {
        perror(("Error closing file after writing: " + filename).c_str());
        throw std::runtime_error("Failed to close file properly after writing: " + filename);
    }

    std::cout << "Memory exported successfully to " << filename << std::endl;
}

void importMemory(const std::string& filename, PageBank* hbmMemory_ptr, int &batchNr, int &totalExecutions, int &totalCorrect, int maxPageNr[BANK_COUNT][TREES_PER_BANK]) {
    // Validate input size consistency
   if (!hbmMemory_ptr) {
       throw std::runtime_error("Null pointer passed for hbmMemory_ptr during load.");
   }

   // Open file in read binary mode
   FILE* fp = fopen(filename.c_str(), "rb");
   if (!fp) {
       perror(("Error opening file for reading: " + filename).c_str());
       throw std::runtime_error("Failed to open file for reading: " + filename);
   }

   // --- File Size Verification (Crucial) ---
   if (fseek(fp, 0, SEEK_END) != 0) {
       perror(("Error seeking to end of file: " + filename).c_str());
       fclose(fp);
       throw std::runtime_error("Failed seeking to end of file: " + filename);
   }
   long file_size_bytes = ftell(fp);
   if (file_size_bytes < 0) {
        perror(("Error getting file size for: " + filename).c_str());
        fclose(fp);
        throw std::runtime_error("Failed to get file size: " + filename);
   }
   if (fseek(fp, 0, SEEK_SET) != 0) {
        perror(("Error seeking to beginning of file: " + filename).c_str());
        fclose(fp);
        throw std::runtime_error("Failed seeking to beginning of file: " + filename);
   }
   // --- End File Size Verification ---
    fread(&batchNr, sizeof(int), 1, fp);
    fread(&totalExecutions, sizeof(int), 1, fp);
    fread(&totalCorrect, sizeof(int), 1, fp);
    fread(&maxPageNr, sizeof(int), BANK_COUNT*TREES_PER_BANK, fp);

   // Cast the PageBank pointer to a node_t pointer to read raw node data directly into memory
   node_t* data_ptr = reinterpret_cast<node_t*>(hbmMemory_ptr);

   // Perform a single large read operation
   size_t items_read = fread(data_ptr,           // Pointer to the start of node_t data destination
                             sizeof(node_t), // Size of one node_t element
                             BANK_COUNT*TREES_PER_BANK*MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE,     // Total number of node_t elements to read
                             fp);                // File pointer

   // --- Error Checking after Read ---
   bool read_error = ferror(fp);
   if (read_error) {
        perror(("Error during reading from file: " + filename).c_str());
        fclose(fp);
        throw std::runtime_error("Error state set during file read: " + filename);
   }
   if (fclose(fp) != 0) {
       perror(("Error closing file after reading: " + filename).c_str());
       std::cerr << "Warning: Failed to close file properly after reading: " << filename << std::endl;
   }

   std::cout << "Memory imported successfully from " << filename << std::endl;
}