#include "converters.hpp"
#include "train.hpp"


node_t nodeToRaw(const Node_hbm &node){
    #pragma HLS inline

    NodeConverter conv(node);
    return conv.raw;
}
Node_hbm rawToNode(const node_t &raw){
    #pragma HLS inline
    NodeConverter conv(raw);
    return conv.node;
}

input_t convertVectorToInput(const input_vector &input){
    #pragma HLS inline
    InputConverter iconv(input);
    return iconv.raw;
}

input_vector convertInputToVector(const input_t &raw){
    #pragma HLS inline
    InputConverter iconv(raw);
    return iconv.input;
}

