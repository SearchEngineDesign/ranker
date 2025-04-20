#include "heuristics.h"

size_t Ranker::distance( Location loc1, Location loc2 )
   {
   return ( loc1 > loc2 ) ? 
      ( loc1 - loc2 ) : 
      ( loc2 - loc1 );  
   }


void Ranker::rarestWord( )
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
         if ( tmpISR->Next( ) == nullptr )
            break;    
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

   std::cout << "rarest: " << rarest << ", occurrence: " << rarestOccurrences << std::endl;

   }   


void Ranker::forward( )
   {
   ISRWord *tmpRarestISR = words[ rarest ];  
   vector< Location > spanLocations( numWords );  // locations of words in a span

   for ( int i = 0; i < rarestOccurrences; i ++ )
      {
      Location rarestWordLocation = tmpRarestISR->GetStartLocation( );  
      spanLocations[ rarest ] = rarestWordLocation;  
      // arrange other ISRs to as close as possible to the rarest word
      for ( int j = 0; j < numWords; j ++ )
         {
         if ( j != rarest )
            {
            ISRWord *tmpOtherISR = words[ j ];  
            int minDifference = INT_MAX;  

            while ( tmpOtherISR != nullptr )
               {
               Location otherWordLocation = tmpOtherISR->GetStartLocation( );  
               int difference = rarestWordLocation - otherWordLocation;  
               // std::cout << "diff: " <<  difference << " other word: " << otherWordLocation << " rarest word: " << rarestWordLocation << std::endl;
               if ( difference > 0 )
                  {
                  minDifference = difference;  
                  spanLocations[ j ] = otherWordLocation;  
                  if ( tmpOtherISR->Next( ) == nullptr )
                     break;  
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

            // reset isr j
            words[ j ]->Seek( docBegin );

            }
         }

      Location nearestLocation = SIZE_MAX, farthestLocation = 0;  // for heuristics
      for ( int i = 0; i < numWords; i ++ )
         {
         // record nearest and farthest location ( for heuristics )
         if ( spanLocations[ i ] < nearestLocation )
            nearestLocation = spanLocations[ i ];  
         if ( spanLocations[ i ] > farthestLocation )
            farthestLocation = spanLocations[ i ];  
         std::cout << spanLocations[i] << " ";
         }

      std::cout << std::endl;
      std::cout << "farthest loc: " << farthestLocation << ", nearest loc: " << nearestLocation << std::endl;

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
      if ( tmpRarestISR->Next( ) == nullptr )  // nullptr if reaching the end of posting list; TODO: memory leak? 
         break;  
      }
   }


int Ranker::dynamicScore( )
   {
   return numShortSpan * shortSpanWeight + 
      numInOrderSpan * inOrderSpanWeight + 
      numExactPhrase * exactPhraseWeight + 
      numTopSpan * topSpanWeight + 
      numMostWordFreq * mostWordFreqWeight;  
   }


int Ranker::staticScore( )
   {
   int isShortTitle, isNiceDocLength, isShortUrl = 0;  

   isShortTitle = endDoc->GetTitleLength( ) <= MaxShortTitle ? 1 : 0;  
   isNiceDocLength = ( endDoc->GetDocumentLength( ) >= MinNiceDocLength && endDoc->GetDocumentLength( ) <= MaxNiceDocLength ) ? 1 : 0;  
   isShortUrl = ( endDoc->GetUrlLength( ) <= MaxShortUrl ) ? 1 : 0;  

   return isShortTitle * shortTitleWeight + 
      isNiceDocLength * docLengthWeight + 
      isShortUrl * shortUrlWeight;  
   }


int Ranker::rankingScore(  )
   {
   rarestWord( );  
   forward( );  
   std::cout << "short span: " << numShortSpan << " , in order span: " << numInOrderSpan << " , exact phrase: " << numExactPhrase << " , top span: " << numTopSpan << " , most word freq: " << numMostWordFreq << "\n";
   return staticScore( ) * staticWeight +  dynamicScore( ) * dynamicWeight;  
   }