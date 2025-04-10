#ifndef __CONVERTERS_HPP__
#define __CONVERTERS_HPP__
#include "common.hpp"
#include "train.hpp"



node_t nodeToRaw(const Node_hbm &node);
Node_hbm rawToNode(const node_t &raw);

input_vector convertInputToVector(const input_t &raw);
input_t convertVectorToInput(const input_vector &input);

union NodeConverter {
    node_t raw;  // Raw representation
    Node_hbm node;                 // Structured representation
    NodeConverter(const Node_hbm &node): node(node){};
    NodeConverter(const node_t &raw): raw(raw) {};
};
template<typename T>
union PropertyConverter {
    node_t raw;
    T p;
    PropertyConverter(const T &p) : p(p){};
    PropertyConverter(const node_t &raw): raw(raw) {};
};


union InputConverter{
    input_t raw;
    input_vector input;
    InputConverter(const input_vector &input) : input(input){};
    InputConverter(const input_t &raw): raw(raw){};
};

template<typename T>
node_t propertiesToRaw(const T &p){
    #pragma HLS inline
    PropertyConverter<T> pconv(p);
    return pconv.raw;
}
template<typename T>
T rawToProperties(const node_t &raw){
    #pragma HLS inline
    PropertyConverter<T> pconv(raw);
    return pconv.p;
}

#endif