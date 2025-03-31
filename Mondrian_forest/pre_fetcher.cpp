#include "train.hpp"
#include "converters.hpp"

void burst_read_page(IPage pageOut, FetchRequest &request, const Page *pagePool);
void update_small_node_bank(hls::stream_of_blocks<trees_t> &smlTreeStream, const Page *pagePool);
//void condense_node(const node_t &from, Node_sml &to, const int currentPage);
void process_tree(FetchRequest &request, hls::stream_of_blocks<IPage> &pageOut, const Page *pagePool, hls::stream_of_blocks<trees_t> &smlTreeStream, hls::stream<bool> &treeUpdateCtrlStream);


void pre_fetcher(hls::stream<FetchRequest> &fetchRequestStream, IPage pageOut, const Page *pagePool)
{
    //int i = 0;
        if(!fetchRequestStream.empty()){
            auto request = fetchRequestStream.read();
            // if(request.updateSmlBank){
            //     update_small_node_bank(smlTreeStream, pagePool);
            //     treeUpdateCtrlStream.write(true);
            // }else{
                burst_read_page(pageOut, request, pagePool);

                //i = (i == TRAVERSAL_BLOCKS - 1) ? 0 : i + 1;
            // }
        }
}

void burst_read_page(IPage pageOut, FetchRequest &request, const Page *pagePool)
{
    #pragma HLS inline
    const int globalPageIdx = request.treeID * MAX_PAGES_PER_TREE + request.pageIdx;
    for(int i = 0; i < MAX_NODES_PER_PAGE; i++){
        #pragma HLS PIPELINE II=1
        pageOut[i] = pagePool[globalPageIdx][i];
    }

    PageProperties p(request.input, request.pageIdx, request.treeID);
    p.freePageIdx = request.freePageIdx;
    pageOut[MAX_NODES_PER_PAGE] = propertiesToRaw(p);
}

void update_small_node_bank(hls::stream_of_blocks<trees_t> &smlTreeStream, const Page *pagePool)
{
    #pragma HLS inline off
    hls::write_lock<trees_t> trees(smlTreeStream);
    update_sml_bank_t: for(int t = 0; t < TREES_PER_BANK; t++){
        update_sml_bank_p: for(int p = 0; p < MAX_PAGES_PER_TREE; p++){
            update_sml_bank_n: for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
                #pragma HLS PIPELINE II=1
                Node_sml sml(rawToNode(pagePool[t*MAX_PAGES_PER_TREE + p][n]), p);
                trees[t][p*MAX_NODES_PER_PAGE+n] = sml;
            }
        }
    }
}

// void condense_node(const node_t &raw, Node_sml &sml, const int currentPage)
// {
//     #pragma hls inline
//     std::cout << "Condense node" << std::endl;
//     const Node_hbm hbm(rawToNode(raw));
//     // if(hbm.valid){
//         sml.leaf(hbm.leaf());
//         if(hbm.leaf()){
//             sml.upperBound(hbm.upperBound[hbm.feature]);
//             sml.lowerBound(hbm.lowerBound[hbm.feature]);
//             for(int i = 0; i < CLASS_COUNT; i++){
//                 sml.classDistribution(i, hbm.classDistribution[i]);
//             }
//         }else{
//             sml.feature(hbm.feature);
//             sml.threshold(hbm.threshold);
//             sml.leftChild((hbm.leftChild.isPage()) ? hbm.leftChild.id()*MAX_NODES_PER_PAGE : hbm.leftChild.id() + currentPage*MAX_NODES_PER_PAGE);
//             sml.rightChild((hbm.rightChild.isPage()) ? hbm.rightChild.id()*MAX_NODES_PER_PAGE : hbm.rightChild.id() + currentPage*MAX_NODES_PER_PAGE);
//         }
        
//         std::cout << "Condense node" << std::endl;
        

//     // }
//     // ap_uint<72> rawsml = *reinterpret_cast<const ap_uint<72>*>(&sml);
//     // smlNodeOutputStream.write(rawsml);
// }