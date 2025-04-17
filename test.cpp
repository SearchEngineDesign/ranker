#include "heuristics.h"
#include "../isr/isr.h"
#include "../index/index.h"
#include "../isr/isrHandler.h"
#include <cstddef>

int main() {

   // read index chunk
   IndexReadHandler readHandler = IndexReadHandler();
   readHandler.ReadIndex("../log/chunks/0");

   // initialize ISRHandler
   ISRHandler isrHandler;
   isrHandler.SetIndexReadHandler(&readHandler);

   char word1[] = "poets";
   ISRWord *isrWord1 = isrHandler.OpenISRWord(word1);
   if (isrWord1 == nullptr) {
      isrHandler.CloseISR(isrWord1);
      return 0;
   }

   int i = 0;
   size_t target = 0;
   while (isrWord1->Seek(target) != nullptr)
   {
      std::cout << "matching doc: " << isrWord1 ->GetMatchingDoc() << std::endl;
      if (i == 10)
         break;
      i++;
      target = isrWord1->EndDoc->GetStartLocation() + 1;
      std::cout << "target: " << target << "\n";


      Ranker ranker;
      ranker.endDoc = isrWord1->EndDoc;
      ranker.numWords = 1;
      ranker.words = &isrWord1;
      ranker.rankingScore();

      std::cout << readHandler.getDocument(isrWord1 ->GetMatchingDoc())->c_str() << std::endl;
   }
   // Close
   isrHandler.CloseISR(isrWord1);

   return 0;

   // ISRWord **words;
   // ISREndDoc *endDoc;
   // int numWords = 2;


}