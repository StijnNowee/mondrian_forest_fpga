#include "train.hpp"

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const int pageIdx, const Page *pagePool);
void update_small_node_bank(hls::stream_of_blocks<trees_t> &treeStream, const int treeID, const Page *pagePool);
void condense_node(const node_t &from, Node_sml &to, int currentPage);
void process_tree(FetchRequest &request, hls::stream_of_blocks<IPage> &pageOut, hls::stream_of_blocks<trees_t> &treeStream, const Page *pagePool);


void pre_fetcher(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, hls::stream_of_blocks<trees_t> &treeStream)
{
    static TreeStatus status[TREES_PER_BANK] = {IDLE};
    static int processCounter[TREES_PER_BANK] = {0};
    int stackptr = 0;
    FetchRequest processStack[TREES_PER_BANK];
    while(!feedbackStream.empty()){
        FetchRequest request = feedbackStream.read();
        if(request.needNewPage){
            processStack[stackptr++] = request;
        } else if(request.done){
            status[request.treeID] = (processCounter[request.treeID]++ == UPDATE_FEQUENCY) ? UPDATING : IDLE;
        }
    }
    for(int t = 0; t < TREES_PER_BANK; t++){
        if(status[t] == IDLE){
            FetchRequest newRequest{splitFeatureStream[t].read(), 0, t, false, false};
            processStack[stackptr++] = newRequest;
        } else if(status[t] == UPDATING){
            FetchRequest newRequest;
            newRequest.updateSmlBank = true;
            processStack[stackptr++] = newRequest;
        }
    }
    for(int sp = 0; sp < stackptr; sp++){
        process_tree(processStack[sp], pageOut, treeStream, pagePool);
    }
}

void process_tree(FetchRequest &request, hls::stream_of_blocks<IPage> &pageOut, hls::stream_of_blocks<trees_t> &treeStream, const Page *pagePool)
{
    if (request.updateSmlBank) {
        hls::write_lock<trees_t> trees(treeStream);
        update_sml_bank_p: for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
            update_sml_bank_n: for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
                condense_node(pagePool[request.treeID*MAX_NODES_PER_PAGE + p][n], trees[request.treeID][p*MAX_NODES_PER_PAGE+n], p);
            }
        }
    }else{
        const int globalPageIdx = request.treeID * MAX_PAGES_PER_TREE + request.pageIdx;
        hls::write_lock<IPage> out(pageOut);
        for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
            out[i] = pagePool[globalPageIdx][i];
        }

        PageProperties p(request.input, request.pageIdx, request.treeID);

        convertPropertiesToRaw(p, out[MAX_NODES_PER_PAGE]);
    }
}

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const int pageIdx, const Page *pagePool)
{
    #pragma HLS inline off
    //Read from memory
    const int globalPageIdx = treeID * MAX_PAGES_PER_TREE + pageIdx;
    hls::write_lock<IPage> out(pageOut);
    for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        out[i] = pagePool[globalPageIdx][i];
    }

    PageProperties p(feature, pageIdx, treeID);

    convertPropertiesToRaw(p, out[MAX_NODES_PER_PAGE]);
}

void update_small_node_bank(hls::stream_of_blocks<trees_t> &treeStream, const int treeID, const Page *pagePool)
{
    #pragma HLS inline off
    hls::write_lock<trees_t> trees(treeStream);
    update_sml_bank: for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
        for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
            condense_node(pagePool[treeID*MAX_NODES_PER_PAGE + p][n], trees[treeID][p*MAX_NODES_PER_PAGE+n], p);
        }
    }
}

void condense_node(const node_t &raw, Node_sml &sml, int currentPage)
{
    Node_hbm hbm;
    convertRawToNode(raw, hbm);
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