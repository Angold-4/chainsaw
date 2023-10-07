//===----------------------------------------------------------------------===//
//
//                         chainsaw
//
// cf.hpp
//
// Identification: src/cf.hpp
//
// Last Modified : 2022.8.24 Jiawei Wang
//
// Copyright (c) 2022 Angold-4
//
//===----------------------------------------------------------------------===//

#pragma once

#include <chrono>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <pthread.h> // multithreading
#include <stdio.h>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#define CFURL "https://codeforces.com/contest/"

// CurlObj is the class to handle url
class CurlObj {
public:
  CurlObj(std::string url) : cfurl(url) {
    curl = curl_easy_init();
    if (!curl)
      throw std::string("Curl did not initialize.");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriter);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlBuffer);
    curl_easy_perform(curl);
  }

  static int curlWriter(char *data, int size, int nmemb, std::string *buffer) {
    // write to the buffer
    int result = 0;
    if (buffer != NULL) {
      buffer->append(data, size * nmemb);
      result = size * nmemb;
    }
    return result;
  }

  std::string getData() { return curlBuffer; }

private:
  CURL *curl;
  std::string cfurl;
  std::string curlBuffer; // write to curlBuffer
};

// Chainsaw is the class to handle cf html
class Chainsaw {
public:
  Chainsaw() {}

  /**
   * Chainsaw::parseProblems()
   *
   * Get the # of problems of this specific test
   * and then parse the prolems blk figure out all the problems name
   * Notice that the passing parameter conthtml is the index page of contest
   */
  int parseProblems(std::string conthtml);

  /**
   * Chainsaw::getProb()
   *
   * Return the vector of all problem name
   */
  std::vector<std::string> getProb() { return this->probnames; }

  /**
   * Chainsaw::parseTests()
   *
   * Parse the pre block
   * and then get # of tests
   * and write each tests to each file
   */
  int parseTests(std::string preblk, std::string prob);

private:
  int ntestcase;
  int nprobcase;
  std::vector<std::string> probnames;
  std::unordered_map<std::string, std::string> replaced_map = {
      {"<br />", "\n"}, {"&lt;", "<"}, {"&gt;", ">"}, {"&amp;", "&"}};

  bool test_cases_used(std::string preblk);
  std::string new_parse_tests(std::string preblk);
  std::string replace(std::string preblk, std::string oldstr,
                      std::string newstr);
};
