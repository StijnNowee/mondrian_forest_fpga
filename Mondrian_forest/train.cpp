#include "train.hpp"


void feature_distributor(hls::stream<input_vector> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], int size)
{
    std::cout << "test" << std::endl;
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
    raw.range(7, 0) = p.pageIdx;
    raw.range(15, 8) = p.nextPageIdx;
    raw.range(23, 16) = p.treeID;
    raw.range(31, 24) = p.freeNodesIdx[0];
    raw.range(39, 32) = p.freeNodesIdx[1];
    raw.range(40, 40) = p.split.enabled;
    raw.range(49, 41) = p.split.nodeIdx;
    raw.range(56, 50) = p.split.dimension;
    raw.range(65, 57) = p.split.parentIdx;
    raw.range(97, 66) = static_cast<uint32_t>(p.split.newSplitTime); // Assuming 32-bit float
    raw.range(98, 98) = p.dontIterate;
    return raw;
}

PageProperties convertRawToProperties(const node_t &raw)
{
    PageProperties p;
    p.pageIdx = raw.range(7, 0);
    p.nextPageIdx = raw.range(15, 8);
    p.treeID = raw.range(23, 16);
    p.freeNodesIdx[0] = raw.range(31, 24);
    p.freeNodesIdx[1] = raw.range(39, 32);
    p.split.enabled = raw.range(40, 40);
    p.split.nodeIdx = raw.range(49, 41);
    p.split.dimension = raw.range(56, 50);
    p.split.parentIdx = raw.range(65, 57);
    p.split.newSplitTime = static_cast<float>(raw.range(97, 66)); // Assuming 32-bit float
    p.dontIterate = raw.range(98, 98);
    return p;
}