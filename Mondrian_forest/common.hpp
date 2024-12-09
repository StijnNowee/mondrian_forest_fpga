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

#define TREES_PER_BANK 3
#define MAX_NODES 100 // Max nodes per bank
#define MAX_DEPTH 50
#define BANK_COUNT 2
#define MAX_PAGES 20

//Page management
#define MAX_NODES_PER_PAGE 5
#define MAX_PAGE_DEPTH 7

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

struct feature_vector{
    unit_interval data[FEATURE_COUNT_TOTAL];
};
//typedef unit_interval feature_vector[FEATURE_COUNT_TOTAL];

struct ChildNode{
    bool isPage = false;
    union{
        int nodeIdx;
        int pageIdx;
    };
};


struct alignas(128) Node_hbm{
    int idx = 0;
    bool leaf = false;
    uint8_t feature = 0;
    unit_interval threshold = 0;
    unit_interval lowerBound[FEATURE_COUNT_TOTAL] = {};
    unit_interval upperBound[FEATURE_COUNT_TOTAL] = {};
    float splittime = 0;
    unit_interval classDistribution[CLASS_COUNT] = {};
    ChildNode leftChild;
    ChildNode rightChild;
    float parentSplitTime = 0;
};

// struct alignas(1024) Page{
//     Node_hbm nodes[MAX_NODES_PER_PAGE];
// };
typedef Node_hbm Page[MAX_NODES_PER_PAGE];

struct Tree{
    int root = 0;
};

#endif