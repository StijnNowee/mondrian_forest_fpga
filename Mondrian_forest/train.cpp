#include "train.hpp"


void feature_distributor(hls::stream<input_vector> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], int size)
{
    for(int i = 0; i < size; i++){
        auto newFeature = newFeatureStream.read();
        for(int t = 0; t < TREES_PER_BANK; t++){
            splitFeatureStream[t].write(newFeature);
        }
    }
}

node_t convertProperties(const PageProperties &p)
{
    node_t raw = 0;
    raw.range(31, 0) = p.pageIdx;
    raw.range(63, 32) = p.nextPageIdx;
    raw.range(95, 64) = p.treeID;
    raw.range(127, 96) = p.freeNodesIdx[0];
    raw.range(159, 128) = p.freeNodesIdx[1];
    raw.range(160, 160) = p.split.enabled;
    raw.range(192, 161) = p.split.nodeIdx;
    raw.range(224, 193) = p.split.dimension;
    raw.range(256, 225) = p.split.parentIdx;
    raw.range(280, 257) = p.split.newSplitTime.range(23, 0);
    raw.range(281, 281) = p.needNewPage;
    raw.range(313, 282) = p.input.label; 
    raw.range(314, 314) = p.extraPage; 
    for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
        raw.range(322 + i*8, 315 + i*8) = p.input.feature[i].range(7,0);
    }
    return raw;
}

PageProperties convertProperties(const node_t &raw)
{
    PageProperties p;
    p.pageIdx =             raw.range(31, 0);
    p.nextPageIdx =         raw.range(63, 32);
    p.treeID =              raw.range(95, 64);
    p.freeNodesIdx[0] =     raw.range(127, 96);
    p.freeNodesIdx[1] =     raw.range(159, 128);
    p.split.enabled =       raw.range(160, 160);
    p.split.nodeIdx =       raw.range(192, 161);
    p.split.dimension =     raw.range(224, 193);
    p.split.parentIdx =     raw.range(256, 225);
    p.split.newSplitTime.range(23, 0) =  raw.range(280, 257);
    p.needNewPage =         raw.range(281, 281);
    p.input.label =         raw.range(313,282);
    p.extraPage =           raw.range(314,314);
    for(int i = 0; i < FEATURE_COUNT_TOTAL; i++){
        p.input.feature[i].range(7,0) = raw.range(322 + i*8, 315 + i*8);
    }
    return p;
}

node_t convertNode(const Node_hbm &node)
{
    node_t raw;
    raw.range(31, 0) = node.idx;
    raw.range(32, 32) = node.leaf;
    raw.range(33, 33) = node.valid; //If this is changed, please change it in findFreeNodes and pageSplit
    raw.range(41, 34) = node.feature;
    raw.range(49,42) = node.threshold.range(7,0);
    raw.range(73,50) = node.splittime.range(23, 0);
    raw.range(97,74) = node.parentSplitTime.range(23, 0);
    raw.range(129,98) = node.leftChild.id;
    raw.range(130,130) = node.leftChild.isPage;
    raw.range(162,131) = node.rightChild.id;
    raw.range(163, 163) = node.rightChild.isPage;
    raw.range(195, 164) = node.labelCount;
    for(int i = 0; i < CLASS_COUNT; i++){
        #pragma HLS UNROLL
        raw.range(202 + i*8,195 + i*8) = node.classDistribution[i].range(7, 0);
    }
    int baseAddress = 203 + 8*(CLASS_COUNT-1);
    for(int j = 0; j < FEATURE_COUNT_TOTAL; j++){
        #pragma HLS UNROLL
        raw.range(baseAddress + 7 + j*8,baseAddress + j*8) = node.lowerBound[j].range(7,0);
        raw.range(baseAddress + 7 + j*8 + FEATURE_COUNT_TOTAL*8, baseAddress + j*8 + FEATURE_COUNT_TOTAL*8) = node.upperBound[j].range(7,0);
    }

    return raw;
}

Node_hbm convertNode(const node_t &raw)
{
    Node_hbm node;
    node.idx = raw.range(31, 0);
    node.leaf = raw.range(32, 32);
    node.valid = raw.range(33, 33);
    node.feature = raw.range(41, 34);
    node.threshold.range(7,0) = raw.range(49,42);
    node.splittime.range(23, 0) = raw.range(73,50);
    node.parentSplitTime.range(23, 0) = raw.range(97,74);
    node.leftChild.id = raw.range(129,98);
    node.leftChild.isPage = raw.range(130,130);
    node.rightChild.id = raw.range(162,131);
    node.rightChild.isPage = raw.range(163, 163);
    node.labelCount = raw.range(195, 164);
    for(int i = 0; i < CLASS_COUNT; i++){
        #pragma HLS UNROLL
        node.classDistribution[i].range(7,0) = raw.range(202 + i*8,195 + i*8);
    }
    int baseAddress = 203 + 8*(CLASS_COUNT-1);
    for(int j = 0; j < FEATURE_COUNT_TOTAL; j++){
        #pragma HLS UNROLL
        node.lowerBound[j].range(7,0) = raw.range(baseAddress + 7 + j*8,baseAddress + j*8);
        node.upperBound[j].range(7,0) = raw.range(baseAddress + 7 + j*8 + FEATURE_COUNT_TOTAL*8, baseAddress + j*8 + FEATURE_COUNT_TOTAL*8);
    }
    return node;
}