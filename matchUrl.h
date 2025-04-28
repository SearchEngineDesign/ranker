#pragma once

#include <cf/searchstring.h>
#include <cf/vec.h>

// normalize tokens (convert to lowercase and remove non-alphanumeric characters)
string normalize(const string &token);

// split a string by a delimiter
vector<string> split(const string &str, char delimiter);

// split a string by delimiter and push to tokens
void splitAdd(const string &str, char delimiter, vector<string> & tokens);

// process domain
void processDomain(const string &domain, vector<string> &tokens);

// parse fragments
void parseFragment(const string &str, vector<string> & tokens);

// parse query parameters and push into tokens
void parsePath(const string &path, vector<string> & tokens);

// tokenize a URL
vector<string> tokenizeUrl(const string &url);

// count number of query matches in url
int matchCount(const vector<string> &tokens, const string &url);