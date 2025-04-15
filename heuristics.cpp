#include "heuristics.h"

size_t DynamicRanker::distance( Location loc1, Location loc2 )
   {
   return ( loc1 > loc2 ) ? 
      ( loc1 - loc2 ) : 
      ( loc2 - loc1 );  
   }


void DynamicRanker::rarestWord( )
   {
   // find locations of matching doc
   docEnd = endDoc->GetStartLocation( );  
   docBegin = docEnd - endDoc->GetDocumentLength( );  

   // number of frequent word
   int numFreqWord = 0;  

   // find rarest word
   for ( int i = 0; i < numWords; i ++ )
      {
      int wordCount = 0;  
      ISRWord *tmpISR = words[ i ];  
      while ( tmpISR != nullptr && tmpISR->GetStartLocation( ) > docBegin && tmpISR->GetStartLocation( ) < docEnd ) 
         {
         wordCount ++; 
         tmpISR->Next( );  
         }
      if ( wordCount < rarestOccurrences )
         {
         rarestOccurrences = wordCount;  
         rarest = i;  
         }
      // whether word freqent
      if ( wordCount >= MinToBeFreq )
         numFreqWord ++;  

      // reset isrs
      words[ i ]->Seek( docBegin );  // TODO: avoid reset
      }

   // increment numMostWordFreq
   if ( ( float ) numFreqWord / numWords >= MinRatioToBeMost )
      numMostWordFreq ++;  

   }   


void DynamicRanker::forward( )
   {
   ISRWord *tmpRarestISR = words[ rarest ];  
   vector< Location > spanLocations( numWords );  // locations of words in a span
   spanLocations[ rarest ] = tmpRarestISR->GetStartLocation( );  

   for ( int i = 0; i < rarestOccurrences; i ++ )
      {
      Location rarestWordLocation = tmpRarestISR->GetStartLocation( );  
      Location nearestLocation = SIZE_MAX, farthestLocation = 0;  // for heuristics
      // arrange other ISRs to as close as possible to the rarest word
      for ( int j = 0; j < numWords; j ++ )
         {
         if ( j != rarest )
            {
            ISRWord *tmpOtherISR = words[ j ];  
            size_t minDifference = SIZE_MAX;  

            while ( tmpOtherISR != nullptr )
               {
               Location otherWordLocation = tmpOtherISR->GetStartLocation( );  
               size_t difference = rarestWordLocation - otherWordLocation;  

               if ( difference > 0 )
                  {
                  minDifference = difference;  
                  spanLocations[ j ] = otherWordLocation;  
                  tmpOtherISR->Next( );  
                  }
               else
                  {
                  // the other word exceeds the rarest word
                  if ( - difference < minDifference )
                     {
                     // if the other word is the closest to the rarest word
                     spanLocations[ j ] = otherWordLocation;  
                     minDifference = - difference;  
                     }
                  break;
                  }
               }
            
            // record nearest and farthest location ( for heuristics )
            if ( spanLocations[ j ] < nearestLocation )
               nearestLocation = spanLocations[ j ];  
            else if ( spanLocations[ j ] > farthestLocation )
               farthestLocation = spanLocations[ j ];  

            // reset isr j
            words[ j ]->Seek( docBegin );

            }
         }

      // calculate heuristics
      // if short span
      if ( farthestLocation - nearestLocation <= MaxToBeShort )
         numShortSpan ++;  
      // in order span; exact phrase; most words are frequent
      bool isInOrder = true, isExactPhrase = true;  
      int numFreqWord = 0;
      for ( int i = 1; i < numWords; i ++ )
         {
         if ( spanLocations[ i - 1 ] > spanLocations[ i ] )
            isInOrder = false;  
         if ( spanLocations[ i - 1 ] != spanLocations[ i ] - 1 )
            isExactPhrase = false;  
         }
      if ( isInOrder )
         numInOrderSpan ++;  
      if ( isExactPhrase )
         numExactPhrase ++;  
      // if span near the top
      if ( farthestLocation <= MinToBeNearTop )
         numTopSpan ++;  


      // look into next occurrence of the rarest word
      tmpRarestISR->Next( );  // nullptr if reaching the end of posting list; TODO: memory leak?
      }
   }


int DynamicRanker::score( )
   {
   return numShortSpan * shortSpanWeight + 
      numInOrderSpan * inOrderSpanWeight + 
      numExactPhrase * exactPhraseWeight + 
      numTopSpan * topSpanWeight + 
      numMostWordFreq * mostWordFreqWeight;  
   }


int DynamicRanker::dynamicRankingScore(  )
   {
   rarestWord( );  
   forward( );  
   return score( );  
   }