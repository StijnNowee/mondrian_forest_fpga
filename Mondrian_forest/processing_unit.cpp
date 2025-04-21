#include "processing_unit.hpp"
#include "train.hpp"
#include "inference.hpp"

void train_control_unit(hls::stream<input_vector> &inputStream, const int &size, Page *pageBank, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], int nextFreePageIdx[TREES_PER_BANK], hls::stream<int> &executionCountStream);
void inference_control_unit(hls::stream<input_vector> &inputStream, hls::stream<ClassSums> &voterOutputStream, const int &size, const Page *pageBank);
void process_feedback(hls::stream<Feedback> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, int maxPageNr[TREES_PER_BANK], int &samplesProcessed);
void process_inference_feedback(hls::stream<IFeedback> &feedbackStream, hls::stream<IFetchRequest> &fetchRequestStream, int &samplesProcessed, ClassSums &sums);
void send_new_requests(input_vector &feature, hls::stream<IFetchRequest> &fetchRequestStream);
void send_new_requests(input_vector &feature, hls::stream<FetchRequest> &fetchRequestStream, const int freePageIndex[TREES_PER_BANK]);

void processing_unit(hls::stream<input_vector> &trainInputStream, hls::stream<input_vector> &inferenceInputStream, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], Page trainPageBank[TREES_PER_BANK*MAX_PAGES_PER_TREE], const Page inferencePageBank[TREES_PER_BANK*MAX_PAGES_PER_TREE], const int sizes[2], hls::stream<ClassSums> &inferenceOutputStream, int maxPageNr[TREES_PER_BANK], hls::stream<int> &executionCountStream)
{
    #pragma HLS DATAFLOW
    inference_control_unit(inferenceInputStream, inferenceOutputStream, sizes[TRAIN], inferencePageBank);
    train_control_unit(trainInputStream, sizes[INF], trainPageBank, rngStream, maxPageNr, executionCountStream);
    
}

void train_control_unit(hls::stream<input_vector> &inputStream, const int &size, Page *pageBank, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], int maxPageNr[TREES_PER_BANK], hls::stream<int> &executionCountStream)
{
    int traverseBlockID = 0, executionCount = 0;

    hls::stream<Feedback,3> feedbackStream("Train feedbackStream");
    hls::stream<FetchRequest,TREES_PER_BANK> fetchRequestStream("Train fetchRequestStream");
    for(int i = 0; i < size; i++){
        input_vector feature = inputStream.read();
        send_new_requests(feature, fetchRequestStream, maxPageNr);
        for(int t = 0; t < TREES_PER_BANK;){
            train(fetchRequestStream, rngStream, feedbackStream, pageBank, traverseBlockID);
            process_feedback(feedbackStream, fetchRequestStream, maxPageNr, t);
            traverseBlockID = (++traverseBlockID == TRAIN_TRAVERSAL_BLOCKS) ? 0 : traverseBlockID;
            executionCount++;
        }
    }
    
    #ifndef IMPLEMENTING
        executionCountStream.write(executionCount);
    #endif
}

void inference_control_unit(hls::stream<input_vector> &inputStream, hls::stream<ClassSums> &voterOutputStream, const int &size, const Page *pageBank)
{
    hls::stream<IFeedback,3> feedbackStream("Inference feedbackStream");
    hls::stream<IFetchRequest,TREES_PER_BANK> fetchRequestStream("Inference fetchRequestStream");

    int traverseBlockID = 0;

    for(int i = 0; i < size; i++){
        input_vector feature = inputStream.read();
        send_new_requests(feature, fetchRequestStream);
        ClassSums sums = {0};
        for(int t = 0 ; t < TREES_PER_BANK;){
            inference(fetchRequestStream, feedbackStream, pageBank, traverseBlockID);
            process_inference_feedback(feedbackStream, fetchRequestStream, t, sums);
            traverseBlockID = (++traverseBlockID == INF_TRAVERSAL_BLOCKS) ? 0 : traverseBlockID;
        }
        voterOutputStream.write(sums);
    }
}

void process_feedback(hls::stream<Feedback> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, int maxPageNr[TREES_PER_BANK], int &samplesProcessed)
{
    if(!feedbackStream.empty()){
        Feedback feedback = feedbackStream.read();
        if(feedback.extraPage){
            maxPageNr[feedback.treeID]++;
        }
        
        if(feedback.needNewPage){
            FetchRequest newRequest(feedback);
            fetchRequestStream.write(newRequest);
        }else{
            samplesProcessed++;
        }
    }
}

void process_inference_feedback(hls::stream<IFeedback> &feedbackStream, hls::stream<IFetchRequest> &fetchRequestStream, int &samplesProcessed, ClassSums &sums)
{
    if(!feedbackStream.empty()){
        IFeedback feedback = feedbackStream.read();
        if(feedback.needNewPage){
            IFetchRequest newRequest(feedback);
            fetchRequestStream.write(newRequest);
        }else{
            samplesProcessed++;
            for(int c = 0; c < CLASS_COUNT; c++){
                #pragma HLS UNROLL
                sums.classSums[c] = sums.classSums[c] + feedback.s.dis[c]; 
            }
        }
    }
}

void send_new_requests(input_vector &feature, hls::stream<IFetchRequest> &fetchRequestStream)
{
    for(int t = 0; t < TREES_PER_BANK; t++){
        IFetchRequest newRequest(feature, 0, t);
        fetchRequestStream.write(newRequest);
    }
}

void send_new_requests(input_vector &feature, hls::stream<FetchRequest> &fetchRequestStream, const int freePageIndex[TREES_PER_BANK])
{
    for(int t = 0; t < TREES_PER_BANK; t++){
        FetchRequest newRequest(feature, 0, t, freePageIndex[t] + 1);
        fetchRequestStream.write(newRequest);
    }
}

void update_weight(Node_hbm &node)
{
    int tmpTotalCounts = 0;
    const bool leaf = node.leaf();
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        tmpTotalCounts += (leaf) ? node.counts[c] : node.getTotalTabs(c);
    }
    ap_ufixed<16, 1> tmpDivision = ap_ufixed<16,1>(1.0) / (tmpTotalCounts + ap_ufixed<8,7>(BETA));
    for(int c = 0; c < CLASS_COUNT; c++){
        #pragma HLS PIPELINE II=1
        node.weight[c] = (((leaf) ? node.counts[c] : node.getTotalTabs(c)) + unit_interval(ALPHA)) * tmpDivision;
    }
}