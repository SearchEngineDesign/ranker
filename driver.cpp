#include "heuristics.h"
#include "../isr/isr.h"
#include "../index/index.h"
#include "../isr/isrHandler.h"
#include <cstddef>
#include "../frontier/ReaderWriterLock.h"
#include <pthread.h>


class QueryDemo 
{
public:
   QueryDemo() {}

   QueryDemo(string& input, ISRHandler & isrHandler, char t) {
      handler = isrHandler;
      // split input into tokens (e.g. input = "quick brown fox")

      string word;

      type = t;

      switch (type) {
         case 'h':
            word = "@";
            break;
         case 't':
            word = "<";
            break;
         case 'b':
            word = "";
            break;
         default:
            word = "";
            break;
      }

      for ( int i = 0; i < input.length(); i ++ ) {
         char c = input[i];
         if ( c == ' ' ) {
            switch (type) {
               case 'b':
                  if ( !word.empty()) {
                     tokens.push_back(word);
                     word = "";
                  }
                  break;
               case 't':
                  if ( !word.empty()) {
                     tokens.push_back(word);
                     word = "<";
                  }
                  break;
               case 'h':
                  if ( !word.empty()) {
                     tokens.push_back(word);
                     word = "@";
                  }
                  break;
               default:
                  if ( !word.empty()) {
                     tokens.push_back(word);
                     word = "";
                  }
                  break;
            }
            
         }
         else {
            word.push_back(c);
         }
      }
      if ( !word.empty() || word != "@" || word != "<" )
         tokens.push_back(word);

      if (tokens.empty()) {
         inIndex = false;
         return;
      }

      for (int i = 0; i < tokens.size(); i ++)
         std::cout << "token: " << tokens[i].c_str() << std::endl;


      // initialize isrword
      for (int i = 0; i < tokens.size(); i ++) {
         ISRWord *isrWord = handler.OpenISRWord( tokens[i].data() );
         // ISRWord *isrWordFlatten = handler.OpenISRWord( tokens[i].data() );
         if (isrWord != nullptr) {
            terms.push_back(isrWord);
            // flattenTerms.push_back(isrWordFlatten);
         }
      }
      if (terms.empty())
         inIndex = false;
      else if (terms.size() > 1){
         isrAnd = handler.OpenISRAnd(terms.data(), terms.size());
         if (isrAnd != nullptr)
            inIndex = true;

         isrPhrase = handler.OpenISRPhrase(terms.data(), terms.size());
         if (isrPhrase != nullptr)
            inIndex = true;
      }
      else {
         inIndex = true;
      }

   }

   ~QueryDemo() {
      for (int i = 0; i < terms.size(); i ++) {
         handler.CloseISR(terms[i]);
         // handler.CloseISR(flattenTerms[i]);
      }
      if (isrAnd != nullptr)
         handler.CloseISR(isrAnd);
      if (isrPhrase != nullptr)
         handler.CloseISR(isrPhrase);
   }

   // return whether words are in index
   bool isInIndex() {
      return inIndex;
   }

   // flattened query
   ISR **flatQuery(size_t target) {
      // for (int i = 0; i < flattenTerms.size(); i ++) {
      //    flattenTerms[i]->Seek(target);
      // }
      // return flattenTerms.data();
      return terms.data();
   }

   // isr to word or isr to And word
   ISR *getISRAnd() {
      if (terms.size() == 1) {
         return terms[0];
      }
      else {
         return isrAnd;
      }
   }

   // isr to word or isr to phrase
   ISR *getISRPhrase() {
      if (terms.size() == 1) {
         return terms[0];
      }
      else {
         return isrPhrase;
      }
   }

   int getNumWords() {
      return terms.size();
   }

   ISRHandler handler; // one handler for one index chunk

private:

   vector<string> tokens;

   vector<ISR*> terms;

   char type;

   // vector<ISR*> flattenTerms; // TODO: protect
   ISRAnd *isrAnd = nullptr;
   ISRPhrase *isrPhrase = nullptr;
   bool inIndex = false;
};


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

vector<unsigned int> shortSpans, inOrderSpans, exactPharses, topSpans, freq;
vector<int> scores;
vector<string> urls;
ReaderWriterLock writerLock;


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
      urls.push_back(readHandler.getDocument(isr ->GetMatchingDoc())->c_str());

      target = isr->EndDoc->GetStartLocation() + 1;
      // std::cout << "target: " << target << "\n";

      Ranker ranker((ISRWord **) query.flatQuery(target), isr->EndDoc, query.getNumWords());
      int score = ranker.rankingScore(writerLock);

      WithWriteLock withWriteLock(writerLock);
      scores.push_back(score);

   }
}


struct SearchArgs {
   const char* fname;  // File name
   string & input; // Input string to search
};


// search query in a specific chunk
void* searchChunk(void *args) {

   SearchArgs* searchArgs = static_cast<SearchArgs*>(args);

   const char* fname = searchArgs->fname;
   string input = searchArgs->input;


   IndexReadHandler readHandler = IndexReadHandler();
   readHandler.ReadIndex(fname);
   ISRHandler handler;
   handler.SetIndexReadHandler(&readHandler);

   QueryDemo query(input, handler, 'h');
   if (!query.isInIndex()) {
      std::cout << "not in index\n";
      return nullptr;
   }

   getRankScore(query, readHandler);


   return nullptr;
}


vector<string> results( string & searchString ) {

   const char* filename = "../log/chunks/8";
   // string searchString = "recipe";
   struct SearchArgs args{filename, searchString};

   
   pthread_t thread1, thread2;

   pthread_create(&thread1, nullptr, searchChunk, &args);
   // pthread_create(&thread2, nullptr, searchChunk, &args);

   pthread_join(thread1, nullptr);

   for (int i = 0; i < scores.size(); i ++) {
      std::cout << "score: " << scores[i] << std::endl;
   }

   return urls;

}

