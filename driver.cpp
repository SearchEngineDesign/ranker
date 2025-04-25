
#include "driver.h"

// TODO: can we use unordered_map

// vector<unsigned int> shortSpans, inOrderSpans, exactPharses, topSpans, freq;

// vector<int> scores;
// vector<string> urls;

// vector<Result> results;


void Driver::getRankScoreQueryCompiler(QueryParser & parser) {
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

         score += ranker.rankingScore();

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

         int scoreTitle = rankerTitle.rankingScore();

         score += scoreTitle * 10;
      }

      WithWriteLock withWriteLock(writerLock);
      results_map[hashbasic(docString)] = {score, docString};
   }
}


// search query in a specific chunk
void* Driver::searchChunk(void *args) {

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

   return nullptr;
}

void* startDriver(void *args) {
   SearchArgs* searchArgs = static_cast<SearchArgs*>(args);
   if (searchArgs->d != nullptr) 
      searchArgs->d->searchChunk(args);
}

// input a search query (searchString), return a vector of urls
vector<string> getResults( string searchString ) {

   Driver drivers[NUM_DRIVERS];
   
   //results_map.clear();

   int chunkCount = 0;
   for (auto& p : std::filesystem::directory_iterator("../log/chunksgoogle"))
      ++chunkCount;

   vector<pthread_t> threads;
   SearchArgs argList[chunkCount];

   int i = 0;
   for (const auto& entry : std::filesystem::directory_iterator("../log/chunksgoogle")) {
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
      


   // sort top 10 results
   vector<Result> sorted_results;
   for (auto &d : drivers) {
      for (const auto& pair : d.results_map) {
         sorted_results.push_back({pair.second.score, pair.second.url});
      }
   }

   std::sort(sorted_results.begin(), sorted_results.end(), Driver::compareResults);

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