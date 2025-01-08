#ifndef COMMON_H_
#define COMMON_H_

#include <ap_int.h>
#include <ap_fixed.h>
#include <cmath>
#include <cstdint>
#include <hls_stream.h>
#include <limits>
#include <iostream>

#define FEATURE_COUNT_TOTAL 2
#define CLASS_COUNT 4

#define TREES_PER_BANK 1
#define MAX_PAGES_PER_TREE 10
#define MAX_NODES 100 // Max nodes per bank
#define MAX_DEPTH 50
#define BANK_COUNT 1
#define MAX_PAGES 20

//Page management
#define MAX_NODES_PER_PAGE 31
#define MAX_PAGE_DEPTH 5

constexpr int log2_ceil(int n, int power = 0) {
    return (n <= (1 << power)) ? power : log2_ceil(n, power + 1);
}
constexpr int INTEGER_BITS = log2_ceil(FEATURE_COUNT_TOTAL);

typedef ap_ufixed<8, 0> unit_interval;
typedef ap_ufixed<INTEGER_BITS + 8, INTEGER_BITS> rate;

typedef ap_uint<8> label_vector;

typedef uint8_t localNodeIdx;

typedef unit_interval feature_vector[FEATURE_COUNT_TOTAL];

struct input_vector {
    feature_vector feature;
    unit_interval label;
};


//typedef unit_interval feature_vector[FEATURE_COUNT_TOTAL];

struct ChildNode{
    bool isPage = false;
    union{
        int nodeIdx;
        int pageIdx;
    };
};

struct FetchRequest{
    input_vector input;
    int pageIdx;
    bool valid = false;
};


struct alignas(128) Node_hbm{
    int idx;
    int parentIdx;
    bool leaf;
    bool valid = false;
    uint8_t feature;
    unit_interval threshold;
    float splittime;
    float parentSplitTime;
    unit_interval lowerBound[FEATURE_COUNT_TOTAL] = {};
    unit_interval upperBound[FEATURE_COUNT_TOTAL] = {};  
    unit_interval classDistribution[CLASS_COUNT] = {};
    ChildNode leftChild = {.nodeIdx = 0};
    ChildNode rightChild = {.nodeIdx = 0};
};

typedef Node_hbm Page[MAX_NODES_PER_PAGE];

struct Tree{
    int root = 0;
};

#endif