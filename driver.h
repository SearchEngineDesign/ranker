#pragma once

#include "../utils/searchstring.h"
#include "../utils/vector.h"
#include "heuristics.h"
#include "../isr/isr.h"
#include "../index/index.h"
#include "../isr/isrHandler.h"


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


struct Result {
   int score;
   string url;

   void print() const {
      std::cout << "Score: " << score << ", URL: " << url << std::endl;
   }
};

vector<string> getResults( string searchString );




