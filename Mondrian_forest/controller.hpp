#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "common.hpp"

void controller(hls::stream<int> &processTreeStream, const Page *pagePool, Node_sml trees[TREES_PER_BANK][MAX_PAGES_PER_TREE*MAX_NODES_PER_PAGE]);

#endif