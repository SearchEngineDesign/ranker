
#include "driver.h"
#include "matchUrl.h"
#include "model.h"

// TODO: can we use unordered_map; BUG: map will give duplicates more scores


void Driver::getRankScoreQueryCompiler(QueryParser & parser, const string & input, char type) {
   writerLock.readLock();
   if (checkStop()) {
      writerLock.readUnlock();
      return;
   }
   writerLock.readUnlock();


   
   ISR *isr = parser.compile();
   if (isr == nullptr) {
      std::cout << "isr nullptr\n";
      return;
   }

   // get token strings from query
   // vector<string> tokens = parser.getTokenStrings(); // TODO: this func doesn't work

   vector<string> tokens = split(input, ' ');
   std::cout << "input: " << input << std::endl;
   size_t target = 0;

   while (isr->Seek(target) != nullptr && !shutdown) {

      //std::cout << "matching doc: " << isr ->GetMatchingDoc() << " " << parser.getIndexReadHandler().getDocument(isr ->GetMatchingDoc())->c_str() << std::endl;
      const char * docString = parser.getIndexReadHandler().getDocument(isr ->GetMatchingDoc())->c_str();
   
      string url(docString);
      size_t urlLength = url.length();

      int score = 0;

      // for dataset
      unsigned int numShortSpan, numInOrderSpan, numExactPhrase, numTopSpan;
      float percentFreqWords;
      size_t docLength; 


      // if body words
      vector<ISRWord*> flatten;
      int weight = 1;
      if (type == 'b') {
         flatten = parser.getFlattenedWords();
      }
      else if (type == 't') {
         flatten = parser.getFlattenedTitles();
         weight = 10;
      }

      // body words
      if (!flatten.empty()) {
         for (int i = 0; i < flatten.size(); i ++) {
            flatten[i]->Seek(isr->EndDoc->GetStartLocation() - isr->EndDoc->GetDocumentLength());
         }

         // new end doc for ranker
         ISREndDoc *endDoc = parser.getISRHandler().OpenISREndDoc();
         endDoc->Seek(isr->EndDoc->GetStartLocation());

         Ranker ranker((ISRWord**) flatten.data(), endDoc, int(flatten.size()), urlLength);

         score += ranker.rankingScore() * weight;

         // for dataset
         numShortSpan = ranker.getNumShortSpan();
         numInOrderSpan = ranker.getNumInOrderSpan();
         numExactPhrase = ranker.getNumExactPhrase();
         numTopSpan = ranker.getNumTopSpan();
         percentFreqWords = ranker.getPercentWordFreq();
         docLength = ranker.getDocLength();

         // close end doc for ranker
         // parser.getISRHandler().CloseISREndDoc(endDoc);
      }

      // match in URL
      score += Ranker::urlScore(tokens, url);
      int matchNum = matchCount(tokens, url);
      int urlMatchWeight = 3;
      score += matchNum * urlMatchWeight;

      // for dataset

      WithWriteLock withWriteLock(writerLock);
      addGoodDocNum(score);

      // results_map[hashbasic(docString)] = {score, docString};
      if (results_map.find(hashbasic(docString)) == results_map.end()) {
         results_map[hashbasic(docString)] = {score, docString, {numShortSpan, 0}, {numInOrderSpan, 0}, {numExactPhrase, 0}, {numTopSpan, 0}, {percentFreqWords, 0.0}, urlLength, docLength, matchNum};
      }
      else {
         Result prevResult = results_map[hashbasic(docString)];
         prevResult.score += score;
         prevResult.numShortSpan.second = numShortSpan;
         prevResult.numInOrderSpan.second = numInOrderSpan;
         prevResult.numExactPhrase.second = numExactPhrase;
         prevResult.numTopSpan.second = numTopSpan;
         prevResult.percentFreqWords.second = percentFreqWords;
      }
   
      target = isr->EndDoc->GetStartLocation() + 1;

   }
}


// search query in a specific chunk
void* Driver::searchChunk(void *args) {

   SearchArgs* searchArgs = static_cast<SearchArgs*>(args);

   const char* fname = searchArgs->fname.c_str();
   string input = searchArgs->input;


   // // MAKE SURE THIS GETS CLEANED UP!
   // IndexReadHandler readHandler = IndexReadHandler();
   // readHandler.ReadIndex(fname);
   // ISRHandler handler;
   // handler.SetIndexReadHandler(&readHandler);

   QueryParser parser(input, 'b');
   if (parser.SetIndexReadHandler(fname) == 0) {
      getRankScoreQueryCompiler(parser, input, 'b');
   }
   

   QueryParser parserTitle(input, 't');
   if (parserTitle.SetIndexReadHandler(fname) == 0) {
      getRankScoreQueryCompiler(parserTitle, input, 't');
   }
   
   return nullptr;
}

void* startDriver(void *args) {
   SearchArgs* searchArgs = static_cast<SearchArgs*>(args);
   if (searchArgs->d != nullptr) 
      searchArgs->d->searchChunk(args);
   return nullptr;
}

// input a search query (searchString), return a vector of urls

string getResults( string searchString ) {


   Driver drivers[NUM_DRIVERS];

   int chunkCount = 0;
   for (auto& p : std::filesystem::directory_iterator("../log/chunks"))
      ++chunkCount;

   vector<pthread_t> threads;
   SearchArgs argList[chunkCount];

   int i = 0;
   for (const auto& entry : std::filesystem::directory_iterator("../log/chunks")) {
      string filename(entry.path().filename().c_str());
      int driverID = i % NUM_DRIVERS;
      //std::cout << "chunk: " << entry.path() << ", driver: " << driverID << std::endl;

      argList[i] = {string(entry.path().c_str()), searchString, &drivers[driverID]};
      pthread_t thread;
      pthread_create(&thread, nullptr, startDriver, &argList[i]);
      threads.push_back(thread);
      ++i;
   }
   sleep(5);
   for (auto &driver : drivers)
      driver.shutdownDriver();
   for (pthread_t &thread : threads)
      pthread_join(thread, 0);

   // sort top 50 results
   vector<Result> sorted_results;
   std::unordered_set<size_t> duplicates;
   for (auto &d : drivers) {
      WithWriteLock wl(d.writerLock);
      for (const auto& pair : d.results_map) {
         if (duplicates.find(pair.first) == duplicates.end()) {
            // sorted_results.push_back({pair.second.score, pair.second.url});
            sorted_results.push_back(pair.second);
            duplicates.insert(pair.first);
         }
      }
   }

   std::sort(sorted_results.begin(), sorted_results.end(), Driver::compareResults);

   for (int i = 0; i < sorted_results.size(); i ++) {
      sorted_results[i].print();
   }


   // Extract the top 50 URLs
   vector<Result> top50Urls;
   for (size_t i = 0; i < std::min(static_cast<size_t>(50), sorted_results.size()); ++i) {
      top50Urls.push_back(sorted_results[i]);
      std::cout << "score: " << top50Urls[i].score << std::endl;
   }

   vector<Result> top10Urls = runModel(top50Urls);

   // Extract the top 10 URLs
   string buf;
   for (size_t i = 0; i < std::min(static_cast<size_t>(10), top10Urls.size()); ++i)
      buf += top10Urls[i].url + "\t" + string(std::to_string(sorted_results[i].score).c_str()) + "\n";

   return buf;
}


// int main() {
//    string query = "university of michigan";
//    string results = getResults(query);

//    std::cout << results << std::endl;

// }