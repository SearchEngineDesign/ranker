#include "matchUrl.h"



// tokenize a URL


// normalize tokens (convert to lowercase and remove non-alphanumeric characters)
string normalize(const string &token) {
   string normalized;
   for (char c : token) {
      if (std::isalnum(c)) {
         // convert to lowercase
         if (c >= 'A' && c <= 'Z') {
            normalized.push_back(c + ('a' - 'A'));
         } 
         else {
            normalized.push_back(c);
         }
      }
      else { // cut characters after non-alphanumeric
         return normalized; // normalized = "": remain characters after non-alphanumeric
      }
   }
   return normalized;
}


// split a string by a delimiter
vector<string> split(const string &str, char delimiter) {
   vector<string> tokens;
   string token = "";

   for (char c : str) {
      if (c == delimiter) {
         if (token != "") {
            // std::cout << "token: " << token << std::endl;
            tokens.push_back(token);
            token = "";
         }
      } 
      else {
         token.push_back(c);
      }
   }
   if (token != "") {
      tokens.push_back(token);
   }
   return tokens;
}


// split a string by delimiter and push to tokens
void splitAdd(const string &str, char delimiter, vector<string> & tokens) {
   string token = "";

   for (char c : str) {
      if (c == delimiter) {
         if (token != "") {
            // std::cout << "token: " << token << std::endl;
            tokens.push_back(normalize(token));
            token = "";
         }
      } 
      else {
         token.push_back(c);
      }
   }
   if (token != "") {
      tokens.push_back(normalize(token));
   }
}


// process domain
void processDomain(const string &domain, vector<string> &tokens) {
   string cleanDomain = domain;

   // remove "www."
   if (cleanDomain.find("www.") != -1) {
      cleanDomain = cleanDomain.substr(4);
   }

   // remove ".com"
   string suffix = cleanDomain.substr(cleanDomain.size() - 4);
   if (cleanDomain.size() > 4 && (suffix == ".com" || suffix == ".org" || suffix == ".net")) {
      cleanDomain = cleanDomain.substr(0, cleanDomain.size() - 4);
   }

   // split by "."
   splitAdd(cleanDomain, '.', tokens);

}


// parse fragments
void parseFragment(const string &str, vector<string> & tokens) {

   vector<string> queries = split(str, '?');

   for (const string &part : queries) {
      if (part.find("+") != -1) {
         splitAdd(part, '+', tokens);
      }
      else if (part.find("-") != -1) {
         splitAdd(part, '-', tokens);
      }
      else if (part.find("_") != -1) {
         splitAdd(part, '_', tokens);
      }
      else {
         string normalized = normalize(part);
         if (normalized != "")
            tokens.push_back(normalized);
      }
   }
}

// TODO: remove this if too slow
// parse query parameters and push into tokens
void parsePath(const string &path, vector<string> & tokens) {
   // parse path
   vector<string> pairs = split(path, '&'); // whether there are queries
   if (pairs.size() > 1) {
      // query type
      for (const string &pair : pairs) {
         vector<string> kv = split(pair, '=');
         for (const string &part : kv) {
            parseFragment(part, tokens);
         }
      }
   }
   else {
      // not query type
      parseFragment(path, tokens);
   }
}


// tokenize a URL
vector<string> tokenizeUrl(const string &url) {
   vector<string> tokens;

   // https://
   size_t schemeEnd = url.find("://");
   // if (schemeEnd != -1) {
   //    tokens.push_back(url.substr(0, schemeEnd)); // Scheme
   // }


   // the rest of the URL (host, path, query, fragment)
   string remainder = url.substr(schemeEnd + 3); // After "://"


   // domain and path split by '/'
   vector<string> parts = split(remainder, '/');


   // the first part of path is the domain
   if (!parts.empty()) {
      processDomain(parts[0], tokens);
   }

   // the rest of the path
   for (size_t i = 1; i < parts.size(); ++i) {
      // std::cout << "path: " << parts[i] << std::endl ;
      parsePath(parts[i], tokens);
   }

   return tokens;
}


// find match in url

// count number of query matches in url
int matchCount(const vector<string> &tokens, const string &url) {
   vector<string> urlTokens = tokenizeUrl(url);
   std::cout << "url tokens: ";
   for (int j = 0; j < urlTokens.size(); j ++) {
      std::cout << urlTokens[j] <<  " ";
   }
   std::cout << "query tokens: " << tokens.size() << std::endl;;
   int count = 0;
   for (int i = 0; i < tokens.size(); i ++) {
      for (int j = 0; j < urlTokens.size(); j ++) {
         if (tokens[i] == urlTokens[j]) {
            count ++;
         }
      }
   }

   return count;
}


// int main() {
//    string url = "https://www.niche.com/colleges/search/best-colleges/s/michigan/";

//    vector<string> tokens = tokenizeUrl(url);

//    for (const string &token : tokens) {
//       std::cout << "token: " << token << std::endl;
//    }
// }

// match queries (remove stop words)


// score 
