#include "train.hpp"

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, input_vector &feature, const int treeID, const int pageIdx, const Page *pagePool);
void update_small_node_bank(hls::stream_of_blocks<trees_t> &treeStream, const int treeID, const Page *pagePool);
void condense_node(const node_t &from, Node_sml &to, int currentPage);


void pre_fetcher(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, hls::stream_of_blocks<trees_t> &treeStream)
{
    //Initialise status
    static TreeStatus status[TREES_PER_BANK] = {IDLE};
    static int scheduled[TREES_PER_BANK] = {2};
    static int updated[TREES_PER_BANK] = {0};
    int nrScheduled = 0;
    // for (int i = 0; i < TREES_PER_BANK; i++) {
    //     status[i] = IDLE;
    //     scheduled[i] = TREES_PER_BANK + 1;
    // }
        // Prioritize processing feedback requests from the feedback stream.
    if(!feedbackStream.empty()){
        FetchRequest request = feedbackStream.read();
        if(request.needNewPage){
            //Tree needs new page
            burst_read_page(pageOut, request.input, request.treeID, request.pageIdx, pagePool);
        }else if (request.done){
            //Tree finished processing
            status[request.treeID] = IDLE;
            if(++updated[request.treeID] == UPDATE_FEQUENCY){
                update_small_node_bank(treeStream, request.treeID, pagePool);
                updated[request.treeID] = 0;
            }
        }
    } else{
        // If no feedback, check for new input vectors for idle trees.
        check_idle_trees: for(int t = 0; t < TREES_PER_BANK; t++){
            if(status[t] == IDLE && !splitFeatureStream[t].empty()){
                scheduled[nrScheduled++] = t;
                //#if(defined __SYNTHESIS__)
                    status[t] = PROCESSING;
                //#endif
            }
        }
        if(nrScheduled > 0){
            scheduled_loop: for(int st = 0; st < nrScheduled; ++st){
                #pragma HLS PIPELINE II=MAX_NODES_PER_PAGE
                auto input = splitFeatureStream[scheduled[st]].read();
                //Fetch the first page for the tree
                burst_read_page(pageOut, input, scheduled[st], 0, pagePool);
            }
            nrScheduled = 0;
        }
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