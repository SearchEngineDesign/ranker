#include "heuristics.h"
#include "../isr/isr.h"
#include "../index/index.h"
#include "../isr/isrHandler.h"
#include <cstddef>

int main() {

   // read index chunk
   IndexReadHandler readHandler = IndexReadHandler();
   readHandler.ReadIndex("../log/chunks/3");

   // initialize ISRHandler
   ISRHandler isrHandler;
   isrHandler.SetIndexReadHandler(&readHandler);

   char word1[] = "summer";
   ISRWord *isrWord1 = isrHandler.OpenISRWord(word1);
   if (isrWord1 == nullptr) {
      std::cout << "no word1\n";
      isrHandler.CloseISR(isrWord1);
      return 0;
   }

   char word2[] = "rain";
   ISRWord *isrWord2 = isrHandler.OpenISRWord(word2);
   if (isrWord2 == nullptr) {
      std::cout << "no word2\n";
      isrHandler.CloseISR(isrWord2);
      return 0;
   }

   char word3[] = "starry";
   ISRWord *isrWord3 = isrHandler.OpenISRWord(word3);
   if (isrWord3 == nullptr) {
      std::cout << "no word3\n";
      isrHandler.CloseISR(isrWord3);
      return 0;
   }

   ISR **terms = new ISR*[3];
   terms[0] = isrWord1;
   terms[1] = isrWord2;
   terms[2] = isrWord3;

   ISRAnd *isrAnd = isrHandler.OpenISRAnd(terms, 3);

   if ( isrAnd == nullptr ) {
      isrHandler.CloseISR(isrAnd);
      return 0;
   }

   int i = 0;
   size_t target = 0;
   while (isrAnd->Seek(target) != nullptr)
   {
      std::cout << "matching doc: " << isrAnd ->GetMatchingDoc() << std::endl;
      if (i == 10)
         break;
      i++;
      target = isrAnd->EndDoc->GetStartLocation() + 1;
      std::cout << "target: " << target << "\n";


      Ranker ranker;
      ranker.endDoc = isrAnd->EndDoc;
      ranker.numWords = 3;
      ranker.words = (ISRWord **) terms;
      std::cout << "1\n";
      ranker.rankingScore();

      std::cout << readHandler.getDocument(isrAnd ->GetMatchingDoc())->c_str() << std::endl;
   }
   // Close
   isrHandler.CloseISR(isrAnd);




   // ISRWord **words;
   // ISREndDoc *endDoc;
   // int numWords = 2;


}