#include "train.hpp"
#include "top_lvl.hpp"
#include <hls_np_channel.h>
#include "hls_task.h"
#include "rng.hpp"
void run_inference(hls::stream<input_t> &inferenceStream, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<ap_uint<50>> &inferenceOutputStreams,  hls::stream<bool> &treeUpdateCtrlStream);
void inference_per_tree(const input_vector &input, const tree_t &tree, hls::stream<ap_uint<50>> &inferenceOutputStream);
void copy_distribution(classDistribution_t &from, ClassDistribution &to);
void shutdown_handler(hls::stream<bool> &controlStream, const int size);

void train(hls::stream<input_t> &inputFeatureStream,Page *pageBank1, const int size, hls::stream<input_t> &inferenceInputStream, hls::stream<ap_uint<50>> &inferenceOutputStream)
{
    #pragma HLS DATAFLOW
    //#pragma HLS INTERFACE ap_ctrl_none port=return
    //#pragma HLS inline
    hls_thread_local hls::stream<FetchRequest,5> feedbackStream("FeedbackStream");
    hls_thread_local hls::stream<FetchRequest,5> fetchRequestStream("FetchRequestStream");
    hls_thread_local hls::stream<input_vector,10> splitFeatureStream[TREES_PER_BANK];
    feature_distributor(inputFeatureStream, splitFeatureStream, size);
    tree_controller(splitFeatureStream, feedbackStream, fetchRequestStream, size);


    hls_thread_local hls::stream_of_blocks<IPage,10> fetchOutput;
    hls_thread_local hls::stream_of_blocks<IPage,3> traverseOutput;
    hls_thread_local hls::stream_of_blocks<IPage,3> pageSplitterOut;
    hls_thread_local hls::stream_of_blocks<IPage,3> nodeSplitterOut;

    hls_thread_local hls::stream_of_blocks<trees_t, 3> treeStream;
    hls_thread_local hls::stream<bool> treeUpdateCtrlStream("TreeUpdateCtrlStream");
    
    //hls_thread_local hls::stream<unit_interval, 100> rngStream[2*BANK_COUNT];
    hls_thread_local hls::split::load_balance<unit_interval, 2> rngStream;

    hls_thread_local hls::stream<bool> controlOutputStreamtest("ControlOuputStreamtest");

    
 

    //hls_thread_local hls::task t2(feature_distributor, inputFeatureStream, splitFeatureStream);
    pre_fetcher(fetchRequestStream, fetchOutput, pageBank1, treeStream, treeUpdateCtrlStream);
    hls_thread_local hls::task t1(rng_generator, rngStream.in);
    hls_thread_local hls::task t3(tree_traversal, fetchOutput, rngStream.out[0], traverseOutput);
    hls_thread_local hls::task t4(page_splitter,traverseOutput, pageSplitterOut);
    hls_thread_local hls::task t5(node_splitter,pageSplitterOut, rngStream.out[1], nodeSplitterOut);
    hls_thread_local hls::task t7(run_inference, inferenceInputStream, treeStream, inferenceOutputStream, treeUpdateCtrlStream);
    save( nodeSplitterOut, feedbackStream, controlOutputStreamtest, pageBank1, size);
    
    //shutdown_handler(controlOutputStreamtest, size);
}

// void shutdown_handler(hls::stream<bool> &controlStream, const int size)
// {
//     for(int i = 0; i < size; i++){
//         controlStream.read();
//     }
// }

void feature_distributor(hls::stream<input_t> &newFeatureStream, hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], const int size)
{
    for(int i = 0; i < size; i++){
        auto rawInput = newFeatureStream.read();
        input_vector newInput;
        convertInputToVector(rawInput, newInput);
        for(int t = 0; t < TREES_PER_BANK; t++){
            std::cout << "writeFeature" << std::endl;
            splitFeatureStream[t].write(newInput);
        }
    }
}

void tree_controller(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<FetchRequest> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, const int size)
{
    TreeStatus status[TREES_PER_BANK] = {IDLE};
    int processCounter[TREES_PER_BANK] = {0};

    for(int i = 0; i < size*TREES_PER_BANK;){
        if(!feedbackStream.empty()){
            std::cout << "read feedback" << std::endl;
            FetchRequest request = feedbackStream.read();
            if(request.needNewPage){
                fetchRequestStream.write(request);
            } else if(request.done){
                status[request.treeID] = (processCounter[request.treeID]++ == UPDATE_FEQUENCY) ? UPDATING : IDLE;
                i++;
            }
        }
        for(int t = 0; t < TREES_PER_BANK; t++){
            if(status[t] == IDLE){
                if(!splitFeatureStream[t].empty()){
                    std::cout << "New Request" << std::endl;
                    FetchRequest newRequest{splitFeatureStream[t].read(), 0, t, false, false};
                    fetchRequestStream.write(newRequest);
                    #ifdef __SYNTHESIS__
                    status[t] = PROCESSING;
                    #else
                    i++;
                    if(processCounter[t]++ == UPDATE_FEQUENCY){
                        newRequest.updateSmlBank = true;
                        fetchRequestStream.write(newRequest);
                    }
                    #endif
                    
                } 
            }else if(status[t] == UPDATING){
                FetchRequest newRequest;
                std::cout << "Update request" << std::endl;
                newRequest.updateSmlBank = true;
                newRequest.treeID = t;
                status[t] = IDLE;
                fetchRequestStream.write(newRequest);
            }
        }
    }
    FetchRequest shutdownRequest;
    shutdownRequest.shutdown = true;
    fetchRequestStream.write(shutdownRequest);
}

void convertNodeToRaw(const Node_hbm &node, node_t &raw){
    raw = *reinterpret_cast<const node_t*>(&node);
}
void convertRawToNode(const node_t &raw, Node_hbm &node){
    *reinterpret_cast<node_t*>(&node) = raw;
}

void convertPropertiesToRaw(const PageProperties &p, node_t &raw){
    raw = *reinterpret_cast<const node_t*>(&p);
}
void convertRawToProperties(const node_t &raw, PageProperties &p){
    *reinterpret_cast<node_t*>(&p) = raw;
}

void write_page(const IPage &localPage, const PageProperties &p, hls::stream_of_blocks<IPage> &pageOut){
    hls::write_lock<IPage> out(pageOut);
    for(int n = 0; n < MAX_NODES_PER_PAGE; n++){
        out[n] = localPage[n];
    }
    convertPropertiesToRaw(p, out[MAX_NODES_PER_PAGE]);
}
void read_page(IPage &localPage, PageProperties &p, hls::stream_of_blocks<IPage> &pageIn){
    hls::read_lock<IPage> in(pageIn);
    for(int n = 0; n < MAX_NODES_PER_PAGE + 1; n++){
        localPage[n] = in[n];
    }
    convertRawToProperties(in[MAX_NODES_PER_PAGE], p);
}

void run_inference(hls::stream<input_t> &inferenceStream, hls::stream_of_blocks<trees_t> &treeStream, hls::stream<ap_uint<50>> &inferenceOutputStream,  hls::stream<bool> &treeUpdateCtrlStream)//hls::stream<ClassDistribution> inferenceOutputstreams[TREES_PER_BANK])
{
    if(!treeStream.empty()){
        hls::read_lock<trees_t> trees(treeStream); 
        treeUpdateCtrlStream.read();
        while(true){
            
            if(!inferenceStream.empty()){
                auto rawInput = inferenceStream.read();
                input_vector newInput;
                convertInputToVector(rawInput, newInput);
                for(int i = 0; i < TREES_PER_BANK; i++){
                    inference_per_tree(newInput, trees[i], inferenceOutputStream);
                }
            }
            if(!treeUpdateCtrlStream.empty()) break;
        }
    }
}

void inference_per_tree(const input_vector &input, const tree_t &tree, hls::stream<ap_uint<50>> &inferenceOutputStream)
{
    bool done = false;
    Node_sml node = tree[0];
    while(!done){

        if(node.leaf){
            done = true;
            ClassDistribution distributionStruct;
            copy_distribution(node.classDistribution, distributionStruct);
            ap_uint<50> output;
            output.range(8,0) = distributionStruct.distribution[0];
            inferenceOutputStream.write(output);
        }else{
            node = (input.feature[node.feature] > node.threshold) ? tree[node.rightChild] : tree[node.leftChild];
        }
    }
}

void copy_distribution(classDistribution_t &from, ClassDistribution &to)
{
    #pragma HLS ARRAY_PARTITION variable=from complete
    #pragma HLS ARRAY_PARTITION variable=to complete

    for(int i = 0; i < CLASS_COUNT; i++)
    {
        #pragma HLS UNROLL
        to.distribution[i] = from[i];
    }
}