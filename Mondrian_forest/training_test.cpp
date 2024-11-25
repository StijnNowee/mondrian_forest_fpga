#include "common.hpp" // Include common definitions and header files
#include "top_lvl.hpp" // Include the top-level function implementation
#include <iostream>
#include <vector>

int main() {
    // Set up streams
    hls::stream<feature_vector> feature_stream;
    hls::stream<label_vector> label_stream;
    //hls::stream<label_vector> output_stream;

    // Generate test input data
    int numFeatures = 1; // Define the number of features for testing
    for (int i = 0; i < numFeatures; ++i) {
        feature_vector feature;
        for(int j=0; j < FEATURE_COUNT_TOTAL; j++){
            feature.data[j] = rand() % 100;
        }
        // Populate other feature fields if necessary
        feature_stream.write(feature);

        label_vector label;
        label = rand() % 10; // Random label for testing
        label_stream.write(label);
    }

    Node_hbm nodeBank1[MAX_NODES];
    Node_hbm nodeBank2[MAX_NODES];


    // Initialize node banks with some dummy values
    for (int i = 0; i < 50; ++i) {
        nodeBank1[i].idx = i;
        nodeBank2[i].idx = i;
        nodeBank1[i].leftChild = i+1;
        nodeBank1[i].rightChild = i+2;

        nodeBank2[i].leftChild = i+1;
        nodeBank2[i].rightChild = i+2;

    }
    
    top_lvl(feature_stream, label_stream, nodeBank1, nodeBank2);
    
    return 0;
}
