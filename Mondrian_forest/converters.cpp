#include "converters.hpp"
#include "train.hpp"


void convertNodeToRaw(const Node_hbm &node, node_t &raw){
    #pragma HLS INLINE

    NodeConverter conv(node);
    raw = conv.raw;

    // raw.range(Offsets::Hbm::idx_end, Offsets::Hbm::idx_start) = node.idx;
    // raw.range(Offsets::Hbm::leaf_end, Offsets::Hbm::leaf_start) = node.leaf;
    // raw.range(Offsets::Hbm::valid_end, Offsets::Hbm::valid_start) = node.valid;
    // raw.range(Offsets::Hbm::feature_end, Offsets::Hbm::feature_start) = node.feature;
    // raw.range(Offsets::Hbm::threshold_end, Offsets::Hbm::threshold_start) = node.threshold;
    // raw.range(Offsets::Hbm::splittime_end, Offsets::Hbm::splittime_start) = node.splittime;
    // raw.range(Offsets::Hbm::parentSplitTime_end, Offsets::Hbm::parentSplitTime_start) = node.parentSplitTime;
    // // feature_vector lowerBound;
    // for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
    //     #pragma HLS PIPELINE II=1
    //     raw.range(Offsets::Hbm::lowerBound_start + Offsets::Hbm::lowerBound_size * i + Offsets::Hbm::lowerBound_size - 1, Offsets::Hbm::lowerBound_start + Offsets::Hbm::lowerBound_size * i) = node.lowerBound[i].range(Offsets::Hbm::lowerBound_size - 1, 0);
    // }
    // // feature_vector upperBound;
    // for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
    // #pragma HLS PIPELINE II=1
    //     raw.range(Offsets::Hbm::upperBound_start + Offsets::Hbm::upperBound_size * i + Offsets::Hbm::upperBound_size- 1, Offsets::Hbm::upperBound_start + Offsets::Hbm::upperBound_size * i) = node.upperBound[i].range(Offsets::Hbm::lowerBound_size - 1, 0);

    // }
    // raw.range(Offsets::Hbm::labelCount_end, Offsets::Hbm::labelCount_start) = node.labelCount;

    // // classDistribution_t classDistribution;
    // for (int i = 0; i < CLASS_COUNT; i++) {
    // #pragma HLS PIPELINE II=1
    //     raw.range(Offsets::Hbm::classDistribution_start + i*Offsets::Hbm::ClassDistribution_size + Offsets::Hbm::ClassDistribution_size-1, Offsets::Hbm::classDistribution_start + i*Offsets::Hbm::ClassDistribution_size) = node.classDistribution[i].range(Offsets::Hbm::ClassDistribution_size -1, 0);
    // }
    // // ChildNode leftChild;
    // raw.range(Offsets::Hbm::leftChild_isPage_end, Offsets::Hbm::leftChild_isPage_start) = node.leftChild.isPage;
    // raw.range(Offsets::Hbm::leftChild_id_end, Offsets::Hbm::leftChild_id_start) = node.leftChild.id;
    // raw.range(Offsets::Hbm::rightChild_isPage_end, Offsets::Hbm::rightChild_isPage_start) = node.rightChild.isPage;
    // raw.range(Offsets::Hbm::rightChild_id_end, Offsets::Hbm::rightChild_id_start) = node.rightChild.id;
    
    // //Fill with 0
    // raw.range(1023, Offsets::Hbm::rightChild_id_end + 1) = 0;
}
void convertRawToNode(const node_t &raw, Node_hbm &node){
    #pragma HLS INLINE
    NodeConverter conv(raw);
    node = conv.structured;
    // node.idx = raw.range(Offsets::Hbm::idx_end, Offsets::Hbm::idx_start);
    // node.leaf = raw.range(Offsets::Hbm::leaf_end, Offsets::Hbm::leaf_start);
    // node.valid = raw.range(Offsets::Hbm::valid_end, Offsets::Hbm::valid_start);
    // node.feature = raw.range(Offsets::Hbm::feature_end, Offsets::Hbm::feature_start);
    // node.threshold = raw.range(Offsets::Hbm::threshold_end, Offsets::Hbm::threshold_start);
    // node.splittime = raw.range(Offsets::Hbm::splittime_end, Offsets::Hbm::splittime_start);
    // node.parentSplitTime = raw.range(Offsets::Hbm::parentSplitTime_end, Offsets::Hbm::parentSplitTime_start);

    // // feature_vector lowerBound;
    // for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
    //     #pragma HLS PIPELINE II=1
    //     // constexpr int offset = Offsets::Hbm::lowerBound_start + i * bit_size(node.lowerBound[0]);
    //     // constexpr int start = offset + bit_size(node.lowerBound[0]) - 1;
    //     node.lowerBound[i].range(Offsets::Hbm::lowerBound_size - 1, 0) = raw.range(Offsets::Hbm::lowerBound_start + Offsets::Hbm::lowerBound_size * i + Offsets::Hbm::lowerBound_size - 1, Offsets::Hbm::lowerBound_start + Offsets::Hbm::lowerBound_size * i);
    // }

    // // feature_vector upperBound;
    // for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
    //     #pragma HLS PIPELINE II=1
    //     // constexpr int offset = Offsets::Hbm::upperBound_start + i * bit_size(node.upperBound[0]);
    //     // constexpr int start = offset + bit_size(node.upperBound[0]) - 1;
    //     node.upperBound[i].range(Offsets::Hbm::lowerBound_size - 1, 0) = raw.range(Offsets::Hbm::upperBound_start + Offsets::Hbm::upperBound_size * i + Offsets::Hbm::upperBound_size- 1, Offsets::Hbm::upperBound_start + Offsets::Hbm::upperBound_size * i);
    // }

    // node.labelCount = raw.range(Offsets::Hbm::labelCount_end, Offsets::Hbm::labelCount_start);

    // // classDistribution_t classDistribution;
    // for (int i = 0; i < CLASS_COUNT; i++) {
    //     #pragma HLS PIPELINE II=1
    //     // constexpr int offset = Offsets::Hbm::classDistribution_start + i * bit_size(node.classDistribution[0]);
    //     // constexpr int start = offset + bit_size(node.classDistribution[0]) - 1;
    //     node.classDistribution[i].range(Offsets::Hbm::ClassDistribution_size -1, 0) = raw.range(Offsets::Hbm::classDistribution_start + i*Offsets::Hbm::ClassDistribution_size + Offsets::Hbm::ClassDistribution_size-1, Offsets::Hbm::classDistribution_start + i*Offsets::Hbm::ClassDistribution_size);
    // }

    // // ChildNode leftChild;
    // node.leftChild.isPage = raw.range(Offsets::Hbm::leftChild_isPage_end, Offsets::Hbm::leftChild_isPage_start);
    // node.leftChild.id = raw.range(Offsets::Hbm::leftChild_id_end, Offsets::Hbm::leftChild_id_start);
    // node.rightChild.isPage = raw.range(Offsets::Hbm::rightChild_isPage_end, Offsets::Hbm::rightChild_isPage_start);
    // node.rightChild.id = raw.range(Offsets::Hbm::rightChild_id_end, Offsets::Hbm::rightChild_id_start);
}

void convertPropertiesToRaw(const PageProperties &p, node_t &raw){
    #pragma HLS INLINE
    raw = 0;
    int bit_offset = 0;

    // input_vector input;
    for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
        #pragma HLS PIPELINE II=1
        raw.range(bit_offset + 7, bit_offset) = p.input.feature[i].range(7,0);
        bit_offset += 8;
    }
    raw.range(bit_offset + CLASS_BITS - 1, bit_offset) = p.input.label;
    bit_offset += CLASS_BITS;
    raw.range(bit_offset, bit_offset) = p.input.inferenceSample;
    bit_offset += 1;

    // bool needNewPage;
    raw.range(bit_offset, bit_offset) = p.needNewPage;
    bit_offset += 1;

    // bool extraPage;
    raw.range(bit_offset, bit_offset) = p.extraPage;
    bit_offset += 1;

    // int pageIdx;
    raw.range(bit_offset + 31, bit_offset) = p.pageIdx;
    bit_offset += 32;

    // int nextPageIdx;
    raw.range(bit_offset + 31, bit_offset) = p.nextPageIdx;
    bit_offset += 32;

    // int treeID;
    raw.range(bit_offset + 31, bit_offset) = p.treeID;
    bit_offset += 32;

    // int freeNodesIdx[2];
    for (int i = 0; i < 2; i++) {
        #pragma HLS PIPELINE II=1
        raw.range(bit_offset + 31, bit_offset) = p.freeNodesIdx[i];
        bit_offset += 32;
    }

    // int freePageIdx;
    raw.range(bit_offset + 31, bit_offset) = p.freePageIdx;
    bit_offset += 32;

    // SplitProperties split;
    raw.range(bit_offset, bit_offset) = p.split.enabled;
    bit_offset += 1;
    raw.range(bit_offset + 31, bit_offset) = p.split.nodeIdx;
    bit_offset += 32;
    raw.range(bit_offset + 31, bit_offset) = p.split.dimension;
    bit_offset += 32;
    raw.range(bit_offset + 31, bit_offset) = p.split.parentIdx;
    bit_offset += 32;
    raw.range(bit_offset + 23, bit_offset) = p.split.newSplitTime.range(23,0);
    bit_offset += 24;
    raw.range(bit_offset + 7, bit_offset) = p.split.rngVal.range(7,0);
    bit_offset += 8;
    
    //raw.range(1023, bit_offset) = 0;
}
void convertRawToProperties(const node_t &raw, PageProperties &p){
    #pragma HLS INLINE
    int bit_offset = 0;

    // input_vector input;
    for (int i = 0; i < FEATURE_COUNT_TOTAL; i++) {
        p.input.feature[i].range(7,0) = raw.range(bit_offset + 7, bit_offset);
        bit_offset += 8;
    }
    p.input.label = raw.range(bit_offset + CLASS_BITS - 1, bit_offset);
    bit_offset += CLASS_BITS;
    p.input.inferenceSample = raw.range(bit_offset, bit_offset);
    bit_offset += 1;

    // bool needNewPage;
    p.needNewPage = raw.range(bit_offset, bit_offset);
    bit_offset += 1;

    // bool extraPage;
    p.extraPage = raw.range(bit_offset, bit_offset);
    bit_offset += 1;

    // int pageIdx;
    p.pageIdx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;

    // int nextPageIdx;
    p.nextPageIdx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;

    // int treeID;
    p.treeID = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;

    // int freeNodesIdx[2];
    for (int i = 0; i < 2; i++) {
        p.freeNodesIdx[i] = raw.range(bit_offset + 31, bit_offset);
        bit_offset += 32;
    }

    // int freePageIdx;
    p.freePageIdx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;

    // SplitProperties split;
    p.split.enabled = raw.range(bit_offset, bit_offset);
    bit_offset += 1;
    p.split.nodeIdx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;
    p.split.dimension = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;
    p.split.parentIdx = raw.range(bit_offset + 31, bit_offset);
    bit_offset += 32;
    p.split.newSplitTime.range(23, 0) = raw.range(bit_offset + 23, bit_offset);
    bit_offset += 24;
    p.split.rngVal.range(7,0) = raw.range(bit_offset + 7, bit_offset);
    bit_offset += 8;
}