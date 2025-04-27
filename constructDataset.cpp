#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include "json.hpp"
#include "matchUrl.h"

#include "driver.h"

// TODO: modify utf8 first (make "new york city" correct)

// TODO: in-link

using json = nlohmann::json;

// construct a document JSON object
json createDocument(const std::string &url, const Result &result, int rank) {
   return {
      {"doc_url", url},
      {"num_short_span", result.numShortSpan},
      {"num_in_order_span", result.numInOrderSpan},
      {"num_exact_phrase", result.numExactPhrase},
      {"num_top_span", result.numTopSpan},
      {"percent_freq_word", result.percentFreqWords},
      {"url_length", result.urlLength},
      {"doc_length", result.docLength},
      {"num_url_match", result.numMatchUrl},
      {"rank", rank}
   };
}

// add a query and documents to the JSON object
void addQuery(json& jsonData, const std::string& query, const vector<Result>& results, const json &jsonLabel) {
   // get the labels
   std::unordered_map<std::string, int> labels;
   for (const auto& label : jsonLabel) {
      if (label.contains("href")) {
         labels[label["href"]] = label["rank"];
      }
   }
   
   // construct documents
   vector<json> documents;
   for (int i = 0; i < results.size(); i ++) {
      // decide rank
      int rank = -1;
      std::string url(results[i].url.c_str());
      auto it = labels.find(url);
      if (it != labels.end()) {
         rank = it->second;
      }

      documents.push_back(createDocument(url, results[i], rank));
   }
   
   // Create the query structure
   json newQuery = {
      {"query", query},
      {"documents", documents}
   };

   // Add it to the "queries" array
   if (!jsonData.contains("queries")) {
      jsonData["queries"] = json::array(); // Initialize "queries" if it doesn't exist
   }
   jsonData["queries"].push_back(newQuery);
}


// write json to file (overwrite)
void writeJsonFile(const json &jsonResult) {
   // Open file in truncation mode to overwrite
   std::ofstream outFile("dataset-test-3.json", std::ios::out | std::ios::trunc);
   if (outFile.is_open()) {
      outFile << jsonResult.dump(4) << std::endl; // Pretty print with 4 spaces
      outFile.close();
      std::cout << "File overwritten successfully with new JSON data." << std::endl;
   } else {
      std::cerr << "Failed to open file for writing." << std::endl;
   }
}


// write json to file (append)
void readJsonFile(json & jsonData) {
   std::ifstream inFile("dataset-test-3.json");

   // Read existing JSON if the file exists
   if (inFile) {
      inFile >> jsonData;
      inFile.close();
   }
   else {
      std::cout << "no json file\n";
      return;
   }

   // Ensure the file contains a JSON array
   if (!jsonData.contains("queries")) {
      jsonData["queries"] = json::array(); // Initialize "queries" if it doesn't exist
   }

}


// dataset-test.json, dataset-2.json, dataset-test-3.json
int main() {
   // read jsonl
   std::string path = "output_full_rank50.jsonl";
   std::ifstream file(path);

   if (!file.is_open()) {
      std::cerr << "Failed to open file: " << path << std::endl;
      return 1;
   }

   // file for checkpoint
   std::ofstream checkpoint("checkpoint.txt");

   // get queries (AND logic); take one query get result and write to json file
   json jsonResult;
   readJsonFile(jsonResult);

   int i = 1;
   std::string line;
   while (std::getline(file, line)) {
      std::cout << i << std::endl;
      if (i < 4801) {
         i ++;
         continue;
      }
      if (i == 4931)
         break;
      try {
            json jsonData = json::parse(line);

            if (jsonData.contains("query")) {
               std::string query = jsonData["query"];
               
               // find matching doc (<=50) and corresponding rank (with heuristics)
               string q(query.c_str());
               char * p = q.c_str();
               for ( ; *p; ++p) *p = tolower(*p);
               vector<Result> results = getResults(q);

               addQuery(jsonResult, query, results, jsonData["results"]);

               if (i % 10 == 0) {
                  writeJsonFile(jsonResult);
                  checkpoint << "complete: " << i << std::endl;
               }

            } else {
               checkpoint << "Missing 'query' field in line: " << line << std::endl;
               checkpoint << "i = " << i << std::endl;
            }
      } catch (const json::exception& e) {
            checkpoint << "JSON parsing error: " << e.what() << " in line: " << line << std::endl;
            checkpoint << "i = " << i << std::endl;
            checkpoint.close();
      }
      i ++;
   }

   // write necessary information to jsonl
   // Open the file in write mode
   file.close(); // close input file

}