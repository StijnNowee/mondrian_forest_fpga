#include "train.hpp"
#include "converters.hpp"

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, FetchRequest &request, const Page *pagePool);
void update_small_node_bank(hls::stream_of_blocks<trees_t> &smlTreeStream, const Page *pagePool);
void condense_node(const node_t &from, Node_sml &to, const int currentPage);
void process_tree(FetchRequest &request, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream);


void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, hls::stream_of_blocks<IPage> pageOut[TRAVERSAL_BLOCKS], const Page *pagePool, hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream)
{
    int i = 0;
    while(true){
        if(!fetchRequestStream.empty()){
            auto request = fetchRequestStream.read();
            if(request.shutdown) break;
            if(request.updateSmlBank){
                update_small_node_bank(smlTreeStream, pagePool);
                treeUpdateCtrlStream.write(true);
            }else{
                burst_read_page(pageOut[i], request, pagePool);

                i = (i == TRAVERSAL_BLOCKS - 1) ? 0 : i + 1;
            }
            
        }
    }
    //process_tree(request, pageOut, treeStream, pagePool, smlNodeOutputStream, treeUpdateCtrlStream);
    // if (request.updateSmlBank) {
    //     std::cout << "Update sml Bank" << std::endl;
    //     hls::write_lock<trees_t> trees(treeStream);
    //     update_sml_bank_p: for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
    //         update_sml_bank_n: for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
    //             condense_node(pagePool[request.treeID*MAX_NODES_PER_PAGE + p][n], trees[request.treeID][p*MAX_NODES_PER_PAGE+n], p, smlNodeOutputStream);
    //         }
    //     }
    //     treeUpdateCtrlStream.write(true);
    // }else{
        // std::cout << "Burst read" << std::endl;
        // const int globalPageIdx = request.treeID * MAX_PAGES_PER_TREE + request.pageIdx;
        // hls::write_lock<IPage> out(pageOut);
        // for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        //     out[i] = pagePool[globalPageIdx][i];
        // }

        // PageProperties p(request.input, request.pageIdx, request.treeID);
        // convertPropertiesToRaw(p, out[MAX_NODES_PER_PAGE]);
    // }
}

// void process_tree(FetchRequest &request, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream)
// {

// }

void burst_read_page(hls::stream_of_blocks<IPage> &pageOut, FetchRequest &request, const Page *pagePool)
{
    #pragma HLS inline
    const int globalPageIdx = request.treeID * MAX_PAGES_PER_TREE + request.pageIdx;
    hls::write_lock<IPage> out(pageOut);
    for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        #pragma HLS PIPELINE II=1
        out[i] = pagePool[globalPageIdx][i];
    }

    PageProperties p(request.input, request.pageIdx, request.treeID);
    p.freePageIdx = request.freePageIdx;
    out[MAX_NODES_PER_PAGE] = propertiesToRaw(p);
}

void update_small_node_bank(hls::stream_of_blocks<trees_t> &smlTreeStream, const Page *pagePool)
{
    #pragma HLS inline off
    hls::write_lock<trees_t> trees(smlTreeStream);
    update_sml_bank_t: for(int t = 0; t < TREES_PER_BANK; t++){
        update_sml_bank_p: for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
            update_sml_bank_n: for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
                #pragma HLS PIPELINE II=1
                condense_node(pagePool[t*MAX_PAGES_PER_TREE + p][n], trees[t][p*MAX_NODES_PER_PAGE+n], p);
            }
        }
    }
}

void condense_node(const node_t &raw, Node_sml &sml, const int currentPage)
{
    #pragma hls inline
    Node_hbm hbm(rawToNode(raw));
    // if(hbm.valid){
        sml.feature = hbm.feature;
        sml.leaf = hbm.leaf();
        sml.threshold.range(7, 0) = hbm.threshold.range(7,0);
        sml.leftChild = (hbm.leftChild.isPage()) ? hbm.leftChild.id()*MAX_NODES_PER_PAGE : hbm.leftChild.id() + currentPage*MAX_NODES_PER_PAGE;
        sml.rightChild = (hbm.rightChild.isPage()) ? hbm.rightChild.id()*MAX_NODES_PER_PAGE : hbm.rightChild.id() + currentPage*MAX_NODES_PER_PAGE;
        for(int i = 0; i < CLASS_COUNT; i++){
            sml.classDistribution[i] = hbm.classDistribution[i];
        }
        sml.upperBound = hbm.upperBound[hbm.feature];
        sml.lowerBound = hbm.lowerBound[hbm.feature];
    // }
    // ap_uint<72> rawsml = *reinterpret_cast<const ap_uint<72>*>(&sml);
    // smlNodeOutputStream.write(rawsml);
}