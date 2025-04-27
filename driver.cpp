
#include "driver.h"
#include "matchUrl.h"

// TODO: can we use unordered_map; BUG: map will give duplicates more scores


void Driver::getRankScoreQueryCompiler(QueryParser & parser, const string & input, char type) {
   ISR *isr = parser.compile();

   if (isr == nullptr)
      return;

   // get token strings from query
   // vector<string> tokens = parser.getTokenStrings(); // TODO: this func doesn't work

   vector<string> tokens = split(input, ' ');

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
         weight = 5;
      }

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
         parser.getISRHandler().CloseISREndDoc(endDoc);
      }

      // // title words
      // vector<ISRWord*> flattenTitles = parser.getFlattenedTitles();

      // if (!flattenTitles.empty()) {
      //    for (int i = 0; i < flattenTitles.size(); i ++) {
      //       flattenTitles[i]->Seek(isr->EndDoc->GetStartLocation() - isr->EndDoc->GetDocumentLength());
      //       std::cout << "title start: " << flattenTitles[i]->GetStartLocation() << std::endl;
      //    }

      //    Ranker rankerTitle((ISRWord**) flattenTitles.data(), isr->EndDoc, int(flattenTitles.size()), urlLength);

      //    int scoreTitle = rankerTitle.rankingScore();

      //    // for dataset
      //    numShortSpan.second = rankerTitle.getNumShortSpan();
      //    numInOrderSpan.second = rankerTitle.getNumInOrderSpan();
      //    numExactPhrase.second = rankerTitle.getNumExactPhrase();
      //    numTopSpan.second = rankerTitle.getNumTopSpan();
      //    percentFreqWords.second = rankerTitle.getPercentWordFreq();

      //    score += scoreTitle * 5;
      // }

      // match in URL
      int matchNum = matchCount(tokens, url);
      int urlMatchWeight = 3;
      score += matchNum * urlMatchWeight;

      // for dataset

      WithWriteLock withWriteLock(writerLock);
      // results_map[hashbasic(docString)] = {score, docString};
      if (results_map.find(hashbasic(docString)) == results_map.end()) {
         std::cout << "new " << type << "\n";
         results_map[hashbasic(docString)] = {score, docString, {numShortSpan, 0}, {numInOrderSpan, 0}, {numExactPhrase, 0}, {numTopSpan, 0}, {percentFreqWords, 0.0}, urlLength, docLength, matchNum};
      }
      else {
         std::cout << "exist " << type << "\n";
         Result prevResult = results_map[hashbasic(docString)];
         prevResult.score += score;
         prevResult.numShortSpan.second = numShortSpan;
         prevResult.numInOrderSpan.second = numInOrderSpan;
         prevResult.numExactPhrase.second = numExactPhrase;
         prevResult.numTopSpan.second = numTopSpan;
         prevResult.percentFreqWords.second = percentFreqWords;
      }

   }
}


// search query in a specific chunk
void* Driver::searchChunk(void *args) {

   SearchArgs* searchArgs = static_cast<SearchArgs*>(args);

   const char* fname = searchArgs->fname.c_str();
   string input = searchArgs->input;

   // MAKE SURE THIS GETS CLEANED UP!
   IndexReadHandler readHandler = IndexReadHandler();
   readHandler.ReadIndex(fname);
   ISRHandler handler;
   handler.SetIndexReadHandler(&readHandler);

   QueryParser parser(input, 'b');
   parser.SetIndexReadHandler(fname);
   getRankScoreQueryCompiler(parser, input, 'b');

   QueryParser parserTitle(input, 't');
   parserTitle.SetIndexReadHandler(fname);
   getRankScoreQueryCompiler(parserTitle, input, 't');
   
   return nullptr;
}

void* startDriver(void *args) {
   SearchArgs* searchArgs = static_cast<SearchArgs*>(args);
   if (searchArgs->d != nullptr) 
      searchArgs->d->searchChunk(args);
   return nullptr;
}

// input a search query (searchString), return a vector of urls
vector<Result> getResults( string searchString ) {

   Driver drivers[NUM_DRIVERS];

   int chunkCount = 0;
   for (auto& p : std::filesystem::directory_iterator("../log/chunks"))
      ++chunkCount;

   vector<pthread_t> threads;
   SearchArgs argList[chunkCount];

   int i = 0;
   for (const auto& entry : std::filesystem::directory_iterator("../log/chunks")) {
      std::cout << entry.path() << std::endl;
      string filename(entry.path().filename().c_str());
      bool digit = true;
      for (int i = 0; i < filename.size(); i++)
         if (!std::isdigit(filename[i]))
            digit = false;
      if (digit) {
         int driverID = atoi(filename.c_str()) % NUM_DRIVERS;
         std::cout << "chunk: " << entry.path() << ", driver: " << driverID << std::endl;

         argList[i] = {string(entry.path().c_str()), searchString, &drivers[driverID]};
         pthread_t thread;
         pthread_create(&thread, nullptr, startDriver, &argList[i]);
         threads.push_back(thread);
      }
      ++i;
   }

   for (pthread_t &thread : threads) {
      pthread_join(thread, nullptr);
   }
      


   // sort top 50 results
   vector<Result> sorted_results;
   for (auto &d : drivers) {
      for (const auto& pair : d.results_map) {
         // sorted_results.push_back({pair.second.score, pair.second.url});
         sorted_results.push_back(pair.second);
      }
   }

   std::sort(sorted_results.begin(), sorted_results.end(), Driver::compareResults);

   for (int i = 0; i < sorted_results.size(); i ++) {
      sorted_results[i].print();
   }

   // // Extract the top 50 URLs
   // vector<string> top10Urls;
   // for (size_t i = 0; i < std::min(static_cast<size_t>(10), sorted_results.size()); ++i) {
   //    top10Urls.push_back(sorted_results[i].url);
   //    // std::cout << "score: " << results[i].score << std::endl;
   // }

   // Extract the top 50 URLs
   vector<Result> top50Urls;
   for (size_t i = 0; i < std::min(static_cast<size_t>(50), sorted_results.size()); ++i) {
      top50Urls.push_back(sorted_results[i]);
      // std::cout << "score: " << results[i].score << std::endl;
   }

   return top50Urls;

}


// int main() {
//    string query = "When is whale watching season in San Diego";
//    vector<Result> results = getResults(query);

//    for (int i = 0; i < results.size(); i ++) {
//       std::cout << results[i].url << std::endl;
//    }

// }