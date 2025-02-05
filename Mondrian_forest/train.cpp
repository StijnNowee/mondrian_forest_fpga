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

node_t convertPropertiesToRaw(const PageProperties &p)
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
    raw.range(280, 257) = p.split.newSplitTime.range(0, 23);
    raw.range(281, 281) = p.dontIterate;
    return raw;
}

PageProperties convertRawToProperties(const node_t &raw)
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
    p.split.newSplitTime.range(0, 23) =  raw.range(280, 257); // Assuming 32-bit float
    p.dontIterate =         raw.range(281, 281);
    return p;
}