#include "model.h"

// Function to load test data from JSON file
vector<vector<float>> constructData(vector<Result> & top50Results) {
   
   vector<vector<float>> queryFeatures;

   for (int i = 0; i < top50Results.size(); i ++) {
      vector<float> features;
      features.push_back((float)top50Results[i].docLength);
      features.push_back((float)top50Results[i].numExactPhrase.first);
      features.push_back((float)top50Results[i].numExactPhrase.second);
      features.push_back((float)top50Results[i].numInOrderSpan.first);
      features.push_back((float)top50Results[i].numInOrderSpan.second);
      features.push_back((float)top50Results[i].numShortSpan.first);
      features.push_back((float)top50Results[i].numShortSpan.second);
      features.push_back((float)top50Results[i].numTopSpan.first);
      features.push_back((float)top50Results[i].numTopSpan.second);
      features.push_back((float)top50Results[i].numMatchUrl);
      features.push_back((float)top50Results[i].percentFreqWords.first);
      features.push_back((float)top50Results[i].percentFreqWords.second);
      features.push_back((float)top50Results[i].urlLength);
      queryFeatures.push_back(features);
   }
   
   return queryFeatures;
}

vector<Result> runModel(vector<Result> & top50Results) {
   try {
      if (!top50Results.size()) {
         vector<Result> out;
         return out;
      }
      // Initialize the model
      RankModel model("rank_model.onnx");

      // get query features
      vector<vector<float>> query_features = constructData(top50Results);

      vector<float> scores = model.predict(query_features);
      
      // Get top 10 documents
      int k = 10;
      vector<Result> topK = model.getTop10(scores, top50Results);
      return topK;

   } catch (const Ort::Exception& e) {
      std::cerr << "ONNX Runtime Error: " << e.what() << std::endl;
   } catch (const std::exception& e) {
      std::cerr << "Error: " << e.what() << std::endl;
   }
}

