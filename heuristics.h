#include "../isr/isr.h"
#include "../isr/isrHandler.h"
#include <climits>

class DynamicRanker 
{
public:
   
   ISRWord **words;  // flatten query words; TODO: seek to the beginning of the matching doc
   ISREndDoc *endDoc;  // endDoc pointing to the matching doc; TODO: seek to the matching doc
   int numWords;  

   void rarestWord( );  // count occurrence of each word in the matching doc and find the rarest; set numMostWordsFreq

   void forward( );  // one pass of isrs; count heuristics

   int score( );  // calculate the score of total dynamic rank

   int dynamicRankingScore( );  // run functions and return the dynamic ranking score of the matching doc

private:

   size_t distance( Location loc1, Location loc2 );  // return distance between loc1 and loc2

   int rarest = 0; // rarest word index
   int rarestOccurrences = INT_MAX;  // num of occurrences of rarest word in matching doc
   Location docBegin, docEnd = 0; // location of matching doc

   // threshold
   const unsigned int MaxToBeShort = 10, MinToBeFreq = 10, MinToBeNearTop = 200;  
   const float MinRatioToBeMost = 0.8;  

   // heuristic
   unsigned int numShortSpan, numInOrderSpan, numExactPhrase, numTopSpan, numMostWordFreq = 0;
   int shortSpanWeight, inOrderSpanWeight, exactPhraseWeight, topSpanWeight, mostWordFreqWeight = 0;
};