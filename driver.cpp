
#include <cstddef>
#include "../frontier/ReaderWriterLock.h"
#include <ostream>
#include <pthread.h>
#include "driver.h"
#include <filesystem>
#include <unordered_map>


// vector<Location> targets; 
// ReaderWriterLock writerLock;


// // multiple threads for finding matching documents
// void findMatchingDoc(QueryDemo & query) {
//    ISR *isr = query.getISR();

//    size_t target = 0;
//    while (isr->Seek(target) != nullptr) {
//       // std::cout << "matching doc: " << isr ->GetMatchingDoc() << std::endl;
//       target = isr->EndDoc->GetStartLocation() + 1;
//       // std::cout << "target: " << target << "\n";

//       // lock write on targets
//       WithWriteLock withWriteLock(writerLock);
//       targets.push_back(target);
//    }
// }


// // single thread for ranking
// void rankDoc(QueryDemo & query) {
//    if (!targets.empty()) {
//       Location target = targets[ targets.size() - 1 ];
//       targets.popBack();

//       // initialize ranker for the doc
//       ISREndDoc *isrEndDoc = new ISREndDoc;
//       isrEndDoc->Seek(target);
//       Ranker ranker((ISRWord **) query.flatQuery(target), isrEndDoc, query.getNumWords());
//       ranker.rankingScore();

//       query.handler.CloseISREndDoc(isrEndDoc);
//    }
// }

// vector<unsigned int> shortSpans, inOrderSpans, exactPharses, topSpans, freq;

// vector<int> scores;
// vector<string> urls;

// vector<Result> results;

static size_t hashbasic(const char *c) 
   {
      unsigned long hash = fnvOffset;
      while (*c) {
         hash *= fnvPrime;
         hash ^= (*c);
         c++;
      }
      return hash % initialSize;
   }

std::unordered_map<size_t, Result> results_map;

ReaderWriterLock writerLock;


bool compareResults(const Result& a, const Result& b) {
   return a.score > b.score;
}



struct SearchArgs {
   string fname;  // File name
   string input; // Input string to search

   // Custom copy assignment operator
   SearchArgs& operator=(const SearchArgs& other) {
      if (this != &other) {
         fname = other.fname;
         input = other.input;
      }
      return *this;
   }
};


void getRankScore(QueryDemo & query, IndexReadHandler & readHandler) {
   ISR *isr = query.getISRAnd();

   size_t target = 0;

   while (isr->Seek(target) != nullptr) {

      std::cout << "matching doc: " << isr ->GetMatchingDoc() << std::endl;
      const char * docString = readHandler.getDocument(isr ->GetMatchingDoc())->c_str();
      
      // writerLock.writeLock();
      // urls.push_back(readHandler.getDocument(isr ->GetMatchingDoc())->c_str());
      // writerLock.writeUnlock();
      target = isr->EndDoc->GetStartLocation() + 1;
      // std::cout << "target: " << target << "\n";
      
      // for url length
      string url(docString);
      size_t urlLength = url.length();

      Ranker ranker((ISRWord **)query.flatQuery(target), isr->EndDoc, query.getNumWords(), urlLength);

      int score = ranker.rankingScore(writerLock);

      WithWriteLock withWriteLock(writerLock);
      // scores.push_back(score);

      // results.push_back({score, docString});
      if (query.getType() == 't')
         results_map[hashbasic(docString)].score += score * 10;
      else
         results_map[hashbasic(docString)].score += score;
      results_map[hashbasic(docString)].url = docString;

   }
}


// search query in a specific chunk
void* searchChunk(void *args) {

   SearchArgs* searchArgs = static_cast<SearchArgs*>(args);

   const char* fname = searchArgs->fname.cstr();
   string input = searchArgs->input;

   IndexReadHandler readHandler = IndexReadHandler();
   readHandler.ReadIndex(fname);
   ISRHandler handler;
   handler.SetIndexReadHandler(&readHandler);

   // for (int i = 0; i < 50; i ++) {
   //    std::cout << "doc: " << readHandler.getDocument(i)->c_str() << std::endl;
   // }


   QueryDemo query(input, handler, 'h'); // TODO: search for all types
   if (query.isInIndex()) {
      getRankScore(query, readHandler);
   }

   QueryDemo query_b(input, handler, 'b'); // TODO: search for all types
   if (query_b.isInIndex()) {
      getRankScore(query_b, readHandler);
   }

   return nullptr;
}



// input a search query (searchString), return a vector of urls
vector<string> getResults( string searchString ) {

   // // clear urls
   // urls.clear();

   // results.clear();

   results_map.clear();


   // multiple threads for chunks

   vector<pthread_t> threads(100);
   vector<SearchArgs> argList(100);

   int i = 0;
   for (const auto& entry : std::filesystem::directory_iterator("../log/chunks")) {
      string filename(entry.path().c_str());
      std::cout << entry.path() << std::endl;

      argList[i] = {filename, searchString};
      pthread_create(&threads[i], nullptr, searchChunk, &argList[i]);
      i ++;
   }

   // for (int i = 0; i < 10; i ++) {
   //    string filename = (string)"../log/chunks/" + to_string(101 + i);
   //    // string filename = (string)"../log/8";
   //    std::cout << filename << std::endl;
   //    argList[i] = {filename, searchString};

   //    pthread_create(&threads[i], nullptr, searchChunk, &argList[i]);
   // }


   for (int j = 0; j < i; j ++) {
      pthread_join(threads[j], nullptr);
   }

   // sort top 10 results
   vector<Result> sorted_results;
   for (const auto& pair : results_map) {
      sorted_results.push_back({pair.second.score, pair.second.url});
   }

   std::sort(sorted_results.begin(), sorted_results.end(), compareResults);

   for (int i = 0; i < sorted_results.size(); i ++) {
      sorted_results[i].print();
   }

   // Extract the top 10 URLs
   vector<string> top10Urls;
   for (size_t i = 0; i < std::min(static_cast<size_t>(10), sorted_results.size()); ++i) {
      top10Urls.push_back(sorted_results[i].url);
      // std::cout << "score: " << results[i].score << std::endl;
   }

   return top10Urls;

}

int main() {
   string str = "york city";
   vector<string> urls = getResults(str);

   for (int i = 0; i < urls.size(); i ++) {
      std::cout << urls[i] << std::endl;
   }
}