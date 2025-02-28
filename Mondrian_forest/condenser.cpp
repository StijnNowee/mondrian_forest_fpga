//#include "train.hpp"
#include "inference.hpp"
//#include <cstring>

void read_and_process_tree(tree_t &tree, const Page *pagePool);
void convertNode(const node_t &from, Node_sml &to, int currentPage);

void convertRawToNode_con(const node_t &raw, Node_hbm &node){
    *reinterpret_cast<node_t*>(&node) = raw;
}

// void controller(hls::stream<int> &processTreeStream, hls::stream<int> &processDoneStream, const Page *pagePool, Node_sml trees[TREES_PER_BANK][MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE])
void condenser(hls::stream<int> &processTreeStream, const Page *pagePool, hls::stream_of_blocks<trees_t> &treeStream)
{
    int treeID = processTreeStream.read();
    hls::write_lock<trees_t> trees(treeStream);
    read_and_process_tree(trees[treeID], pagePool);
        //processDoneStream.write(treeID);
    // processTreeStream.read();
    // for(int i=0; i < TREES_PER_BANK; i++){
    //     read_and_process_tree(trees[i], pagePool);
    // }
}

void read_and_process_tree(tree_t &tree, const Page *pagePool)
{
    for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
        for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
            convertNode(pagePool[p][n], tree[p*MAX_NODES_PER_PAGE+n], p);
        }
    }
}

void convertNode(const node_t &raw, Node_sml &sml, int currentPage)
{
    Node_hbm hbm;
    //memcpy(&hbm, &raw, sizeof(Node_hbm));
    convertRawToNode_con(raw, hbm);
    sml.feature = hbm.feature;
    sml.leaf = hbm.leaf;
    sml.threshold.range(7, 0) = hbm.threshold.range(7,0);
    sml.leftChild = (hbm.leftChild.isPage) ? hbm.leftChild.id*MAX_NODES_PER_PAGE : hbm.leftChild.id + currentPage*MAX_NODES_PER_PAGE;
    sml.rightChild = (hbm.rightChild.isPage) ? hbm.rightChild.id*MAX_NODES_PER_PAGE : hbm.rightChild.id + currentPage*MAX_NODES_PER_PAGE;
    for(int i = 0; i < CLASS_COUNT; i++){
        if(hbm.classDistribution[i] > 0.996){
            sml.classDistribution[i] = -1;
        }else{
            sml.classDistribution[i] = hbm.classDistribution[i];
        }
    }
}