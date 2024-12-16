#include "common.hpp" // Include common definitions and header files
#include "top_lvl.hpp" // Include the top-level function implementation
#include <iostream>
#include <vector>

int main() {
    // Set up streams
    hls::stream<feature_vector> feature_stream;

    // Generate test input data
    int numFeatures = 2; // Define the number of features for testing
    for (int i = 0; i < numFeatures; ++i) {
        feature_vector feature;
        for(int j=0; j < FEATURE_COUNT_TOTAL; j++){
            feature.data[j] = rand() % 100;
        }
        // Populate other feature fields if necessary
        feature_stream.write(feature);

        label_vector label;
        label = rand() % 10; // Random label for testing
        //label_stream.write(label);
    }

    Page pageBank1[MAX_PAGES];
    Page pageBank2[MAX_PAGES];


    // Initialize node banks with some dummy values
    for (int i = 0; i < MAX_NODES_PER_PAGE; ++i) {
        pageBank1[0][i].idx = i;
        pageBank2[0][i].idx = i;
        pageBank1[0][i].leftChild.nodeIdx = i+1;
        pageBank1[0][i].rightChild.nodeIdx = i+1;

        pageBank2[0][i].leftChild.nodeIdx = i+1;
        pageBank2[0][i].rightChild.nodeIdx = i+1;
    }
    
    top_lvl(feature_stream, pageBank1, pageBank2);
    
    return 0;
}
