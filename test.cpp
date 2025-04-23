#include "heuristics.h"
#include "../isr/isr.h"
#include "../index/index.h"
#include "../isr/isrHandler.h"
#include <cstddef>

int main() {

   // read index chunk
   IndexReadHandler readHandler = IndexReadHandler();
   readHandler.ReadIndex("../log/chunks/new");

   // initialize ISRHandler
   ISRHandler isrHandler;
   isrHandler.SetIndexReadHandler(&readHandler);

   char word1[] = "wiki";
   ISRWord *isrWord1 = isrHandler.OpenISRWord(word1);
   if (isrWord1 == nullptr) {
      std::cout << "no word1\n";
      isrHandler.CloseISR(isrWord1);
      return 0;
   }

   // char word2[] = "rain";
   // ISRWord *isrWord2 = isrHandler.OpenISRWord(word2);
   // if (isrWord2 == nullptr) {
   //    std::cout << "no word2\n";
   //    isrHandler.CloseISR(isrWord2);
   //    return 0;
   // }

   // char word3[] = "hot";
   // ISRWord *isrWord3 = isrHandler.OpenISRWord(word3);
   // if (isrWord3 == nullptr) {
   //    std::cout << "no word3\n";
   //    isrHandler.CloseISR(isrWord3);
   //    return 0;
   // }

   // ISR **terms = new ISR*[3];
   // terms[0] = isrWord1;
   // terms[1] = isrWord2;
   // terms[2] = isrWord3;

   // ISRAnd *isrAnd = isrHandler.OpenISRAnd(terms, 3);

   // if ( isrAnd == nullptr ) {
   //    isrHandler.CloseISR(isrAnd);
   //    return 0;
   // }

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


      Ranker ranker((ISRWord **) &isrWord1, isrWord1->EndDoc, 1);

      std::cout << "1\n";
      ReaderWriterLock writerLock;
      ranker.rankingScore(writerLock);

      std::cout << readHandler.getDocument(isrWord1 ->GetMatchingDoc())->c_str() << std::endl;
   }
   // Close
   isrHandler.CloseISR(isrWord1);




   // ISRWord **words;
   // ISREndDoc *endDoc;
   // int numWords = 2;


}