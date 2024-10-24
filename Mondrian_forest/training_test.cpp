#include "top_lvl.hpp"
#include <hls_stream.h>
#include <iostream>

#define FEATURE_COUNT_TOTAL 4  // Define the number of features
#define TREE_COUNT 2           // Number of trees for the testbench

// Testbench main function
int main() {
    // Define input streams
    hls::stream<feature_vector> inputFeatureStream;
    hls::stream<label_vector> inputLabelStream;
    hls::stream<label_vector> outputStream;

    // Create some sample input data for features and labels
    feature_vector sampleFeature;
    label_vector sampleLabel;
    
    for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
        sampleFeature.data[i] = i * 0.1;  // Example feature values
    }
    sampleLabel = 1;  // Example label value

    // Push input data into streams
    for (int i = 0; i < 10; i++) {
        inputFeatureStream.write(sampleFeature);
        inputLabelStream.write(sampleLabel);
    }

    // Call the top-level function to process the input data
    top_lvl(inputFeatureStream, inputLabelStream, outputStream);

    // Read and print output data - limit to expected number of outputs
    for (int i = 0; i < TREE_COUNT; i++) {
        if (!outputStream.empty()) {
            label_vector outputLabel = outputStream.read();
            std::cout << "Output Label: " << outputLabel << std::endl;
        } else {
            std::cout << "Output Stream is empty for tree " << i << std::endl;
        }
    }

    return 0;
}