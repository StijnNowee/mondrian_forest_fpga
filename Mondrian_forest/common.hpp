#ifndef COMMON_H_
#define COMMON_H_

#include <ap_int.h>
#include <ap_fixed.h>
#include <cmath>
#include <cstdint>
#include <hls_stream.h>
#include <limits>
#include <iostream>

#define FEATURE_COUNT_TOTAL 44
#define CLASS_COUNT 7

#define TREE_COUNT 2
#define MAX_NODES 100 // Max nodes per bank
#define MAX_DEPTH 50

constexpr int log2_ceil(int n, int power = 0) {
    return (n <= (1 << power)) ? power : log2_ceil(n, power + 1);
}
constexpr int INTEGER_BITS = log2_ceil(FEATURE_COUNT_TOTAL);

typedef ap_ufixed<8, 0> unit_interval;
typedef ap_ufixed<INTEGER_BITS + 8, INTEGER_BITS> rate;

typedef ap_uint<8> label_vector;

typedef uint8_t localNodeIdx;

struct input_vector {
    unit_interval data[FEATURE_COUNT_TOTAL];
    unit_interval label;
};

struct feature_vector {
    unit_interval data[FEATURE_COUNT_TOTAL];
};


struct alignas(128) Node_hbm{
    int idx;
    bool leaf;
    uint8_t feature;
    unit_interval threshold;
    unit_interval lowerBound[FEATURE_COUNT_TOTAL];
    unit_interval upperBound[FEATURE_COUNT_TOTAL];
    float splittime;
    unit_interval classDistribution[CLASS_COUNT] = {};
    int leftChild;
    int rightChild;
    int parent;
    Node_hbm() : feature(-1), threshold(0.0), leftChild(0), rightChild(0), splittime(0.0), leaf(false){}
    // Node_hbm(bool leaf, uint8_t feature = -1, unit_interval threshold = 0.0, int leftChild = -1, int rightChild = -1, unit_interval splittime = 0.0)
    // : leaf(leaf), feature(feature), threshold(threshold), leftChild(leftChild), rightChild(rightChild), splittime(splittime) {}
};


struct Tree{
    int root = 0;
};

#endif