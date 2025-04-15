#include "processing_unit.hpp"
#include "train.hpp"
#include "inference.hpp"

void feature_distributor(hls::stream<input_vector> &newInputStream, hls::stream<input_vector> splitInputStream[TREES_PER_BANK], const int size);
void train_control_unit(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], const int &size, Page *pageBank, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], int nextFreePageIdx[TREES_PER_BANK], hls::stream<int> &executionCountStream);
void inference_control_unit(hls::stream<input_vector> &splitFeatureStream, hls::stream<ClassSums> &voterOutputStream, const int &size, const Page *pageBank);
void send_new_request(hls::stream<input_vector> &splitFeatureStream, hls::stream<FetchRequest> &fetchRequestStream, const int &treeID, const int &freePageIndex);
void process_feedback(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<Feedback> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, int maxPageNr[TREES_PER_BANK], int &samplesProcessed, ap_uint<TREES_PER_BANK> &processing);
void process_inference_feedback(hls::stream<IFeedback> &feedbackStream, hls::stream<IFetchRequest> &fetchRequestStream, const input_vector &feature, int &samplesProcessed, ap_uint<TREES_PER_BANK> &processing, ClassSums &sums);
void sendOutput(hls::stream<input_vector> &inputStream,
               hls::stream<ClassSums> &voterOutputStream, ClassSums &sums,
               int &samplesProcessed, input_vector &feature, int &i);


void processing_unit(hls::stream<input_vector> &trainInputStream, hls::stream<input_vector> &inferenceInputStream, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], Page *trainPageBank, const Page *inferencePageBank, const InputSizes &sizes, hls::stream<ClassSums> &inferenceOutputStream, int maxPageNr[TREES_PER_BANK], hls::stream<int> &executionCountStream)
{
    #pragma HLS DATAFLOW
    hls::stream<input_vector,3> trainTreeInputStream[TREES_PER_BANK], inferenceTreeInputStream[TREES_PER_BANK];
    feature_distributor(trainInputStream, trainTreeInputStream, sizes.seperate[TRAIN]);
    //feature_distributor(inferenceInputStream, inferenceTreeInputStream, sizes.seperate[INF]);
    inference_control_unit(inferenceInputStream, inferenceOutputStream, sizes.seperate[INF], inferencePageBank);

    train_control_unit(trainTreeInputStream, sizes.seperate[TRAIN], trainPageBank, rngStream, maxPageNr, executionCountStream);
    
   
}

void feature_distributor(hls::stream<input_vector> &newInputStream, hls::stream<input_vector> splitInputStream[TREES_PER_BANK], const int size)
{
    for(int i = 0; i < size; i++){
        auto input = newInputStream.read();
        for(int t = 0; t < TREES_PER_BANK; t++){
            #pragma HLS PIPELINE II=1
            splitInputStream[t].write(input);
        }
    }
    
}

void train_control_unit(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], const int &size, Page *pageBank, hls::stream<unit_interval> rngStream[TRAIN_TRAVERSAL_BLOCKS], int maxPageNr[TREES_PER_BANK], hls::stream<int> &executionCountStream)
{
    ap_uint<TREES_PER_BANK> processing = 0;
    int traverseBlockID = 0, executionCount = 0;

    hls::stream<Feedback,TREES_PER_BANK> feedbackStream("Train feedbackStream");
    hls::stream<FetchRequest,TREES_PER_BANK> fetchRequestStream("Train fetchRequestStream");
    for(int i = 0; i < size*TREES_PER_BANK;){
        executionCount++;
        process_feedback(splitFeatureStream, feedbackStream, fetchRequestStream, maxPageNr, i, processing);
        traverseBlockID = (++traverseBlockID == TRAIN_TRAVERSAL_BLOCKS) ? 0 : traverseBlockID;
        train(fetchRequestStream, rngStream, feedbackStream, pageBank, traverseBlockID);
    }
    //#ifdef TIMINGTEST
        executionCountStream.write(executionCount);
    //#endif
}

void inference_control_unit(hls::stream<input_vector> &inputStream, hls::stream<ClassSums> &voterOutputStream, const int &size, const Page *pageBank)
{
    ap_uint<TREES_PER_BANK> processing = 0;
    int traverseBlockID = 0;

    ClassSums sums;
    int samplesProcessed = 0;
    
    input_vector feature;
    inputStream.read_nb(feature);

    hls::stream<IFeedback,TREES_PER_BANK> feedbackStream("Inference feedbackStream");
    hls::stream<IFetchRequest,TREES_PER_BANK> fetchRequestStream("Inference fetchRequestStream");
    for(int i = 0; i < size*TREES_PER_BANK;){
        process_inference_feedback(feedbackStream, fetchRequestStream, feature, samplesProcessed, processing, sums);
        sendOutput(inputStream, voterOutputStream, sums, samplesProcessed, feature, i);
        traverseBlockID = (++traverseBlockID == INF_TRAVERSAL_BLOCKS) ? 0 : traverseBlockID;
        inference(fetchRequestStream, feedbackStream, pageBank, traverseBlockID);
    }
}

void sendOutput(hls::stream<input_vector> &inputStream,
               hls::stream<ClassSums> &voterOutputStream, ClassSums &sums,
               int &samplesProcessed, input_vector &feature, int &i) {
    if (samplesProcessed == TREES_PER_BANK) {
        samplesProcessed = 0;
        i++;
        voterOutputStream.write(sums);
        if (!inputStream.empty())
            feature = inputStream.read();
        for(int c = 0; c < CLASS_COUNT; c++){
            sums.classSums[c] = 0;
        }
    }
}

void process_feedback(hls::stream<input_vector> splitFeatureStream[TREES_PER_BANK], hls::stream<Feedback> &feedbackStream, hls::stream<FetchRequest> &fetchRequestStream, int maxPageNr[TREES_PER_BANK], int &samplesProcessed, ap_uint<TREES_PER_BANK> &processing)
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
            processing[feedback.treeID] = false;
        }
    }
    for(int t = 0; t < TREES_PER_BANK; t++){
        #pragma HLS PIPELINE II=2

        if(processing[t] == false && !splitFeatureStream[t].empty() && !fetchRequestStream.full()
        #ifndef __SYNTHESIS__
        && fetchRequestStream.empty()
        #endif
        ){
            processing[t] = true;
            send_new_request(splitFeatureStream[t], fetchRequestStream, t, maxPageNr[t] + 1);
        }
    }
}

void process_inference_feedback(hls::stream<IFeedback> &feedbackStream, hls::stream<IFetchRequest> &fetchRequestStream, const input_vector &feature, int &samplesProcessed, ap_uint<TREES_PER_BANK> &processing, ClassSums &sums)
{
    if(!feedbackStream.empty()){
        IFeedback feedback = feedbackStream.read();
        if(feedback.needNewPage){
            IFetchRequest newRequest(feedback);
            fetchRequestStream.write(newRequest);
        }else{
            samplesProcessed++;
            processing[feedback.treeID] = false;
            for(int c = 0; c < CLASS_COUNT; c++){
                sums.classSums[c] = sums.classSums[c] + feedback.s.dis[c]; 
            }
        }
    }
    for(int t = 0; t < TREES_PER_BANK; t++){
        #pragma HLS PIPELINE II=2
        if(processing[t] == false && !fetchRequestStream.full()
        #ifndef __SYNTHESIS__
        && fetchRequestStream.empty()
        #endif
        ){
            processing[t] = true;
            IFetchRequest newRequest(feature, 0, t);
            fetchRequestStream.write(newRequest);
        }
    }
}

void send_new_request(hls::stream<input_vector> &splitFeatureStream, hls::stream<FetchRequest> &fetchRequestStream, const int &treeID, const int &freePageIndex)
{
    FetchRequest newRequest(splitFeatureStream.read(), 0, treeID, freePageIndex);
    fetchRequestStream.write(newRequest);
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