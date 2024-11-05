#include "common.hpp" // Include common definitions and header files
#include "top_lvl.hpp" // Include the top-level function implementation
#include <iostream>
#include <vector>

int main() {
    // Testbench for verifying the top-level model

    // Step 1: Set up test input data streams and output data stream
    hls::stream<feature_vector> feature_stream;
    hls::stream<label_vector> label_stream;
    hls::stream<label_vector> output_stream;

    // Step 2: Generate test input data
    int numFeatures = 10; // Define the number of features for testing
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

    // Step 3: Set up node banks (assuming a fixed size for demonstration)
    Node_hbm nodeBank1[1024];
    Node_hbm nodeBank2[1024];

    // Initialize node banks with some dummy values (if needed)
    for (int i = 0; i < 1024; ++i) {
        nodeBank1[i].idx = i;
        nodeBank2[i].idx = i;
    }

    // Step 4: Call the top-level function for testing
    top_lvl(feature_stream, label_stream, nodeBank1, nodeBank2, output_stream);

    // Step 5: Read and print the outputs for verification
    // while (!output_stream.empty()) {
    //     label_vector result = output_stream.read();
    //     std::cout << "Result: " << result.label_value << std::endl; // Assuming label_vector has an overloaded << operator
    // }

    // Additional validation can be done here to compare results with expected values
    
    return 0;
}
