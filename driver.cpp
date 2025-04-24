
#include <cstddef>
#include "../frontier/ReaderWriterLock.h"
#include <ostream>
#include <pthread.h>
#include "driver.h"
#include <filesystem>
// #include <unordered_map>
#include "../queryCompiler/compiler.h"


// vector<unsigned int> shortSpans, inOrderSpans, exactPharses, topSpans, freq;

// vector<int> scores;
// vector<string> urls;

vector<Result> results;

// static size_t hashbasic(const char *c) 
//    {
//       unsigned long hash = fnvOffset;
//       while (*c) {
//          hash *= fnvPrime;
//          hash ^= (*c);
//          c++;
//       }
//       return hash % initialSize;
//    }

// std::unordered_map<size_t, Result> results_map;

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

// // for QueryDemo
// void getRankScore(QueryDemo & query, IndexReadHandler & readHandler) {
//    ISR *isr = query.getISRAnd();

//    size_t target = 0;

//    while (isr->Seek(target) != nullptr) {

//       std::cout << "matching doc: " << isr ->GetMatchingDoc() << std::endl;
//       const char * docString = readHandler.getDocument(isr ->GetMatchingDoc())->c_str();
      
//       // writerLock.writeLock();
//       // urls.push_back(readHandler.getDocument(isr ->GetMatchingDoc())->c_str());
//       // writerLock.writeUnlock();
//       target = isr->EndDoc->GetStartLocation() + 1;
//       // std::cout << "target: " << target << "\n";
      
//       // for url length
//       string url(docString);
//       size_t urlLength = url.length();

//       Ranker ranker((ISRWord **)query.flatQuery(target), isr->EndDoc, query.getNumWords(), urlLength);

//       int score = ranker.rankingScore(writerLock);

//       WithWriteLock withWriteLock(writerLock);
//       // scores.push_back(score);

//       // results.push_back({score, docString});
//       if (query.getType() == 't')
//          results_map[hashbasic(docString)].score += score * 10;
//       else
//          results_map[hashbasic(docString)].score += score;
//       results_map[hashbasic(docString)].url = docString;

//    }
// }


void getRankScoreQueryCompiler(QueryParser & parser) {
   ISR *isr = parser.compile();

   if (isr == nullptr)
      return;

   size_t target = 0;

   while (isr->Seek(target) != nullptr) {

      std::cout << "matching doc: " << isr ->GetMatchingDoc() << " " << parser.getIndexReadHandler().getDocument(isr ->GetMatchingDoc())->c_str() << std::endl;
      const char * docString = parser.getIndexReadHandler().getDocument(isr ->GetMatchingDoc())->c_str();
      
      target = isr->EndDoc->GetStartLocation() + 1;
      // std::cout << "target: " << target << "\n";
      
      // for url length
      string url(docString);
      size_t urlLength = url.length();

      int score = 0;

      // body words
      vector<ISRWord*> flatten = parser.getFlattenedWords();
      if (!flatten.empty()) {
         for (int i = 0; i < flatten.size(); i ++) {
            flatten[i]->Seek(isr->EndDoc->GetStartLocation() - isr->EndDoc->GetDocumentLength());
         }

         // new end doc for ranker
         ISREndDoc *endDoc = parser.getISRHandler().OpenISREndDoc();
         endDoc->Seek(isr->EndDoc->GetStartLocation());

         Ranker ranker((ISRWord**) flatten.data(), endDoc, int(flatten.size()), urlLength);

         score += ranker.rankingScore(writerLock);

         // close end doc for ranker
         parser.getISRHandler().CloseISREndDoc(endDoc);
      }

      // title words
      vector<ISRWord*> flattenTitles = parser.getFlattenedTitles();

      if (!flattenTitles.empty()) {
         for (int i = 0; i < flattenTitles.size(); i ++) {
            flattenTitles[i]->Seek(isr->EndDoc->GetStartLocation() - isr->EndDoc->GetDocumentLength());
            std::cout << "start: " << flattenTitles[i]->GetStartLocation() << std::endl;
         }

         Ranker rankerTitle((ISRWord**) flattenTitles.data(), isr->EndDoc, int(flattenTitles.size()), urlLength);

         int scoreTitle = rankerTitle.rankingScore(writerLock);

         score += scoreTitle * 10;
      }

      WithWriteLock withWriteLock(writerLock);
      results.push_back({score, docString});

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


   QueryParser parser(input);
   parser.SetIndexReadHandler(fname);
   getRankScoreQueryCompiler(parser);

   // QueryDemo query(input, handler, 't'); // TODO: search for all types
   // if (query.isInIndex()) {
   //    getRankScore(query, readHandler);
   // }

   // QueryDemo query_b(input, handler, 'b'); // TODO: search for all types
   // if (query_b.isInIndex()) {
   //    getRankScore(query_b, readHandler);
   // }

   return nullptr;
}



// input a search query (searchString), return a vector of urls
vector<string> getResults( string searchString ) {

   results.clear();

   const char *CHUNK_DIR = "../log/chunks";

   int chunkCount = 0;
   for (auto& p : std::filesystem::directory_iterator(CHUNK_DIR))
      ++chunkCount;

   vector<pthread_t> threads(chunkCount);
   vector<SearchArgs> argList(chunkCount);

   int i = 0;
   for (const auto& entry : std::filesystem::directory_iterator(CHUNK_DIR)) {
      string filename(entry.path().c_str());
      std::cout << entry.path() << std::endl;

      argList[i] = {filename, searchString};
      pthread_create(&threads[i], nullptr, searchChunk, &argList[i]);
      ++i;
   }

   for (int j = 0; j < i; j ++) {
      pthread_join(threads[j], nullptr);
   }

   std::sort(results.begin(), results.end(), compareResults);

   for (int i = 0; i < results.size(); i ++) {
      results[i].print();
   }

   // Extract the top 10 URLs
   vector<string> top10Urls;
   for (size_t i = 0; i < std::min(static_cast<size_t>(10), results.size()); ++i) {
      top10Urls.push_back(results[i].url);
      // std::cout << "score: " << results[i].score << std::endl;
   }

   return top10Urls;

}


// int main() {
//    string str = "university michigan";
//    vector<string> urls = getResults(str);


//    for (int i = 0; i < urls.size(); i ++) {
//       std::cout << urls[i] << std::endl;
//    }
// }