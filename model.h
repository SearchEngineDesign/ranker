#pragma once
#include "onnxruntime-linux-x64-gpu-1.21.1/include/onnxruntime_cxx_api.h"
#include <cf/searchstring.h>
#include <cf/vec.h>
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include "driver.h"


class RankModel {
private:
   Ort::Env env;
   Ort::Session session{nullptr};
   vector<const char*> input_names;
   vector<const char*> output_names;
   Ort::AllocatorWithDefaultOptions allocator;

public:
   RankModel(const string& model_path) : env(ORT_LOGGING_LEVEL_WARNING, "RankModel") {
      // Create session options
      Ort::SessionOptions session_options;
      session_options.SetIntraOpNumThreads(1);
      session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

      // Load the ONNX model
      session = Ort::Session(env, model_path.c_str(), session_options);

      // Get input and output names
      Ort::AllocatorWithDefaultOptions allocator;
      input_names.push_back("input");
      output_names.push_back("output");
   }

   vector<float> predict(const vector<vector<float>>& doc_features) {
      // Convert input to tensor
      size_t batch_size = 1;
      size_t num_docs = doc_features.size();
      size_t num_features = doc_features[0].size();

      vector<int64_t> input_shape;
      input_shape.push_back(static_cast<int64_t>(batch_size));
      input_shape.push_back(static_cast<int64_t>(num_docs));
      input_shape.push_back(static_cast<int64_t>(num_features));
      
      // Flatten the input data
      vector<float> input_data;
      for (const auto& doc : doc_features) {
         for (const auto& feature : doc) {
            input_data.push_back(feature);
         }
      }

      // Create input tensor
      auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
      Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
         memory_info, input_data.data(), input_data.size(), 
         input_shape.data(), input_shape.size());

      // Run inference
      auto output_tensors = session.Run(
         Ort::RunOptions{nullptr}, 
         input_names.data(), &input_tensor, 1, 
         output_names.data(), 1);

      // Get the output
      float* output_data = output_tensors[0].GetTensorMutableData<float>();
      size_t output_size = output_tensors[0].GetTensorTypeAndShapeInfo().GetElementCount();
      
      // Convert output to vector
      vector<float> predictions; 
      predictions.reserve(output_size);

      for (size_t i = 0; i < output_size; ++i) {
         predictions.push_back(output_data[i]);
      }
      return predictions;
   }

   vector<Result> getTop10(const vector<float>& scores, vector<Result> & top50Results) {
      // Create vector of indices
      vector<int> indices(scores.size());
      for (size_t i = 0; i < scores.size(); ++i) {
         indices[i] = static_cast<int>(i);
      }

      // Sort indices based on scores in descending order
      std::sort(indices.begin(), indices.end(), 
               [&scores](int a, int b) { return scores[a] > scores[b]; });

      // Get the top 10 URLs
      vector<Result> top10Urls;
      for (int i = 0; i < 10 && i < static_cast<int>(indices.size()); ++i) {
         top10Urls.push_back(top50Results[indices[i]]);
      }

      return top10Urls;
   }
};


vector<vector<float>> constructData(vector<Result> & top50Results);

vector<Result> runModel(vector<Result> & top50Results);