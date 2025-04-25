#pragma once

#include "../utils/searchstring.h"
#include "../utils/vector.h"
#include "heuristics.h"
#include "../isr/isr.h"
#include "../index/index.h"
#include "../isr/isrHandler.h"
#include <unordered_map>
#include "../queryCompiler/compiler.h"
#include <filesystem>
#include <cstddef>
#include "../frontier/ReaderWriterLock.h"
#include <ostream>
#include <pthread.h>

const uint8_t NUM_DRIVERS = 12;

static size_t hashbasic(const char *c) 
   {
      unsigned long hash = fnvOffset;
      while (*c) {
         hash *= fnvPrime;
         hash ^= (*c);
         c++;
      }
      return hash % initialSize;
   }

struct Result {
   int score;
   string url;

   void print() const {
      std::cout << "Score: " << score << ", URL: " << url << std::endl;
   }
};

vector<string> getResults( string searchString );

class Driver {
public:

   static bool compareResults(const Result& a, const Result& b) {
      return a.score > b.score;
   }

   void* searchChunk(void *args);
   std::unordered_map<size_t, Result> results_map;

private:
   void getRankScoreQueryCompiler(QueryParser & parser);

   ReaderWriterLock writerLock;

};



struct SearchArgs {
   string fname;  // File name
   string input; // Input string to search
   Driver *d;

   // Custom copy assignment operator
   SearchArgs() : fname(""), input(""), d(nullptr) {};
   SearchArgs(const string &fname_in, const string &input_in, Driver *d_in) 
            : fname(fname_in), input(input_in), d(d_in) {}
   SearchArgs& operator=(const SearchArgs& other) {
      if (this != &other) {
         fname = other.fname;
         input = other.input;
         d = other.d;
      }
      return *this;
   }
};