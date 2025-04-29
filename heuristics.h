#include "../isr/isr.h"
#include "../isr/isrHandler.h"
#include <climits>
#include "../frontier/ReaderWriterLock.h"


class Ranker 
{
public:

   Ranker( ISRWord **w, ISREndDoc *e, int num, size_t urlLen ) {
      words = w;
      endDoc = e;
      numWords = num;
      urlLength = urlLen;
   }
   
   ~Ranker() { }

   void rarestWord( );  // count occurrence of each word in the matching doc and find the rarest; set numMostWordsFreq

   void forward( );  // one pass of isrs; count heuristics

   int staticScore( );  // calculate static score
   int dynamicScore( );  // calculate dynamic score; TODO: title, url, head, body

   static int urlScore(vector<string> &tokens, string &url);  // determine occurences of token in url

   int rankingScore( );  // run functions and return the ranking score of the matching doc

   vector<unsigned int> getHeuristics( );

   unsigned int getNumShortSpan();
   unsigned int getNumInOrderSpan();
   unsigned int getNumExactPhrase();
   unsigned int getNumTopSpan();
   float getPercentWordFreq();
   size_t getDocLength();

private:


   ISRWord **words;  // flatten query words; TODO: seek to the beginning of the matching doc
   ISREndDoc *endDoc;  // endDoc pointing to the matching doc; TODO: seek to the matching doc
   int numWords; 

   size_t distance( Location loc1, Location loc2 );  // return distance between loc1 and loc2

   int rarest = 0; // rarest word index
   int rarestOccurrences = INT_MAX;  // num of occurrences of rarest word in matching doc
   Location docBegin, docEnd = 0; // location of matching doc

   // threshold
   const unsigned int MaxToBeShort = 10, MinToBeFreq = 10, MinToBeNearTop = 200;  
   const float MinRatioToBeMost = 0.8;  

   const unsigned int MaxShortTitle = 20, MinNiceDocLength = 500, MaxNiceDocLength = 2000, MaxShortUrl = 20;  

   // static heuristic
   size_t urlLength = 0, docLength = 0;   
   int shortTitleWeight = 0, docLengthWeight = 0, shortUrlWeight = 0;  

   // dynamic heuristic
   unsigned int numShortSpan = 0, numInOrderSpan = 0, numExactPhrase = 0, numTopSpan = 0, numMostWordFreq = 0;
   float percentWordFreq = 0.0;
   int shortSpanWeight = 2, inOrderSpanWeight = 3, exactPhraseWeight = 5, topSpanWeight = 3, mostWordFreqWeight = 3;

   // static & dynamic weight
   int staticWeight = 0, dynamicWeight = 0;  
};