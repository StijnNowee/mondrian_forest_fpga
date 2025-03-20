#ifndef __CONVERTERS_HPP__
#define __CONVERTERS_HPP__
#include "common.hpp"
#include "train.hpp"

node_t propertiesToRaw(const PageProperties &p);
PageProperties rawToProperties(const node_t &raw);

node_t nodeToRaw(const Node_hbm &node);
Node_hbm rawToNode(const node_t &raw);

union NodeConverter {
    node_t raw;  // Raw representation
    Node_hbm node;                 // Structured representation
    NodeConverter(const Node_hbm &node): node(node){};
    NodeConverter(const node_t &raw): raw(raw) {};
};

union PropertyConverter {
    node_t raw;
    PageProperties p;
    PropertyConverter(const PageProperties &p) : p(p){};
    PropertyConverter(const node_t &raw): raw(raw) {};
};



#endif