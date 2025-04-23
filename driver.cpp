
#include <cstddef>
#include "../frontier/ReaderWriterLock.h"
#include <ostream>
#include <pthread.h>
#include "driver.h"
#include <filesystem>


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

vector<Result> results;

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
   ISR *isr = query.getISRPhrase();

   size_t target = 0;
   int i = 0;
   while (isr->Seek(target) != nullptr) {

      if (i == 10)
         break;
      i ++;

      std::cout << "matching doc: " << isr ->GetMatchingDoc() << std::endl;
      std::cout << readHandler.getDocument(isr ->GetMatchingDoc())->c_str() << std::endl;
      
      // writerLock.writeLock();
      // urls.push_back(readHandler.getDocument(isr ->GetMatchingDoc())->c_str());
      // writerLock.writeUnlock();

      target = isr->EndDoc->GetStartLocation() + 1;
      // std::cout << "target: " << target << "\n";

      Ranker ranker((ISRWord **) query.flatQuery(target), isr->EndDoc, query.getNumWords());
      int score = ranker.rankingScore(writerLock);

      WithWriteLock withWriteLock(writerLock);
      // scores.push_back(score);
      results.push_back({score, readHandler.getDocument(isr ->GetMatchingDoc())->c_str()});

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

   QueryDemo query(input, handler, 'b'); // TODO: search for all types
   if (!query.isInIndex()) {
      std::cout << "not in index\n";
      return nullptr;
   }

   getRankScore(query, readHandler);


   return nullptr;
}


inline void reverse_string(string& str) 
{
if (str.empty()) return;
size_t left = 0;
size_t right = str.size() - 1;
while (left < right) 
   {
      // Swap characters at left and right positions
      char temp = str[left];
      str[left] = str[right];
      str[right] = temp;
      // Move inward from both ends
      ++left;
      --right;
   }
} 


string to_string(int n)
   {
   if (n == 0) return "0";
   bool negative = n < 0;
   string temp;
   if (negative) n = -n;
   while (n > 0) 
      {
      temp.push_back( (char)(n % 10 + '0') );
      n /= 10;
      }
   if (negative) 
      temp.push_back('-');
   reverse_string( temp );
   return temp;
   }


// input a search query (searchString), return a vector of urls
vector<string> getResults( string searchString ) {

   // // clear urls
   // urls.clear();

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
   std::sort(results.begin(), results.end(), compareResults);

   // Extract the top 10 URLs
   vector<string> top10Urls;
   for (size_t i = 0; i < std::min(static_cast<size_t>(10), results.size()); ++i) {
      top10Urls.push_back(results[i].url);
      // std::cout << "score: " << results[i].score << std::endl;
   }

   return top10Urls;

}

// int main() {
//    string str = "government";
//    vector<string> urls = getResults(str);

//    for (int i = 0; i < urls.size(); i ++) {
//       std::cout << urls[i] << std::endl;
//    }
// }