#ifndef __CONVERTERS_HPP__
#define __CONVERTERS_HPP__
#include "common.hpp"

class PageProperties;

void convertPropertiesToRaw(const PageProperties &p, node_t &raw);
void convertRawToProperties(const node_t &raw, PageProperties &p);

void convertNodeToRaw(const Node_hbm &node, node_t &raw);
void convertRawToNode(const node_t &raw, Node_hbm &node);

template <typename T>
constexpr int bit_size() {
    return sizeof(T) * 8;
}

union NodeConverter {
    node_t raw;  // Raw representation
    Node_hbm structured;                 // Structured representation
    NodeConverter(const Node_hbm &node): structured(node){};
    NodeConverter(const node_t &raw): raw(raw) {};
};

namespace Offsets {
    namespace Hbm {
        constexpr int idx_start = 0;
        constexpr int idx_end = idx_start + bit_size<decltype(Node_hbm::idx)>() - 1;
        constexpr int leaf_start = idx_end + 1;
        constexpr int leaf_end = leaf_start + bit_size<decltype(Node_hbm::leaf)>() - 1;
        constexpr int valid_start = leaf_end + 1;
        constexpr int valid_end = valid_start + bit_size<decltype(Node_hbm::valid)>() - 1;
        constexpr int feature_start = valid_end + 1;
        constexpr int feature_end = feature_start + bit_size<decltype(Node_hbm::feature)>() - 1;
        constexpr int threshold_start = feature_end + 1;
        constexpr int threshold_end = threshold_start + bit_size<decltype(Node_hbm::threshold)>() - 1;
        constexpr int splittime_start = threshold_end + 1;
        constexpr int splittime_end = splittime_start + bit_size<decltype(Node_hbm::splittime)>() - 1;
        constexpr int parentSplitTime_start = splittime_end + 1;
        constexpr int parentSplitTime_end = parentSplitTime_start + bit_size<decltype(Node_hbm::parentSplitTime)>() - 1;
        constexpr int lowerBound_start = parentSplitTime_end + 1;
        constexpr int lowerBound_size = bit_size<decltype(Node_hbm::lowerBound[0])>();
        constexpr int lowerBound_end = lowerBound_start + (FEATURE_COUNT_TOTAL * bit_size<unit_interval>()) - 1;
        constexpr int upperBound_start = lowerBound_end + 1;
        constexpr int upperBound_size = bit_size<decltype(Node_hbm::upperBound[0])>();
        constexpr int upperBound_end = upperBound_start + (FEATURE_COUNT_TOTAL * bit_size<unit_interval>()) - 1;
        constexpr int labelCount_start = upperBound_end + 1;
        constexpr int labelCount_end = labelCount_start + bit_size<decltype(Node_hbm::labelCount)>() - 1;
        constexpr int classDistribution_start = labelCount_end + 1;
        constexpr int ClassDistribution_size = bit_size<decltype(Node_hbm::classDistribution[0])>();
        constexpr int classDistribution_end = classDistribution_start + (CLASS_COUNT * bit_size<ap_ufixed<9, 1>>()) - 1;
        constexpr int leftChild_isPage_start = classDistribution_end + 1;
        constexpr int leftChild_isPage_end = leftChild_isPage_start + bit_size<decltype(ChildNode::isPage)>() - 1;
        constexpr int leftChild_id_start = leftChild_isPage_end + 1;
        constexpr int leftChild_id_end = leftChild_id_start + bit_size<decltype(ChildNode::id)>() - 1;
        constexpr int rightChild_isPage_start = leftChild_id_end + 1;
        constexpr int rightChild_isPage_end = rightChild_isPage_start + bit_size<decltype(ChildNode::isPage)>() - 1;
        constexpr int rightChild_id_start = rightChild_isPage_end + 1;
        constexpr int rightChild_id_end = rightChild_id_start + bit_size<decltype(ChildNode::id)>() - 1;

    }
}

#endif