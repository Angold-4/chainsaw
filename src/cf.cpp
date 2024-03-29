//===----------------------------------------------------------------------===//
//                         chainsaw
//
// cf.cpp
//
// Identification: src/cf.cpp
//
// Last Modified : 2022.9.30 Jiawei Wang
//
// Copyright (c) 2022 Angold-4
//
//===----------------------------------------------------------------------===//

/*
 * MIT License
 *
 * Copyright (c) 2022 Jiawei Wang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "./cf.hpp"

static int curlWriter(char *data, int size, int nmemb, std::string *buffer) {
  // write to the buffer
  int result = 0;
  if (buffer != NULL) {
    buffer->append(data, size * nmemb);
    result = size * nmemb;
  }
  return result;
}

std::unordered_map<std::string, std::string>
    preblks; // all preblks for multithreading

void *curlPre(void *url) {
  // For multithreading
  CURL *curl;
  std::string curlBuffer;

  std::string *sp = static_cast<std::string *>(url);
  std::string URL = *sp;

  delete sp;

  curl = curl_easy_init();
  if (!curl)
    throw std::string("Curl did not initialize.");
  curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriter);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlBuffer);
  curl_easy_perform(curl);

  int fpre = curlBuffer.find("<pre>");
  int lpre = curlBuffer.rfind("</pre>", fpre);

  int pl = URL.rfind('/'); // problem number in the last
  std::string prbname = URL.substr(pl + 1, URL.size() - pl);
  preblks[prbname] = curlBuffer.substr(fpre, lpre - fpre) +
                     "</pre>"; // set the global variable
  pthread_exit(NULL);
}

int Chainsaw::parseProblems(std::string html) {
  int foption = html.find("<option value=\"A\" >");
  int lselect = html.find("</select>", foption);
  std::string optblk = html.substr(foption, lselect - foption);

  int count = 0;
  size_t found = 0;
  size_t probindex = 0;
  size_t eindex = 0;
  // store the name of problems
  this->probnames = {};
  while (true) {
    found = optblk.find("\n", found + 1);
    if (found == std::string::npos)
      break;
    probindex = optblk.find("e=", probindex + 1);
    eindex = optblk.find("\" >", probindex);
    probindex += 3;
    std::string pname = optblk.substr(probindex, eindex - probindex);
    this->probnames.push_back(pname);
    count++;
  }
  return count;
}

int Chainsaw::parseTests(std::string preblk, std::string prob) {
  int ntests = 0;
  size_t spre = 0; // start pre
  size_t epre = 0; // end pre
  bool isTest = true;

  while (true) {
    spre = preblk.find("<pr", spre);
    if (spre == std::string::npos)
      break;
    spre += 5;
    epre = preblk.find("</pr", spre);

    if (spre < epre) {
      // valid <pre> block
      ntests++;
      std::string test;
      std::string blk = preblk.substr(spre, epre - spre);
      // ofstream never create dir
      if (isTest) {
        isTest = false;

        std::string blk = preblk.substr(spre, epre - spre);

        // <pre> block
        test = "sample/" + prob + "/input" + std::to_string(ntests / 2 + 1) +
               ".txt";

      } else {
        isTest = true;
        test =
            "sample/" + prob + "/output" + std::to_string(ntests / 2) + ".txt";
      }

      // for each key in replaced_map, check whether blk contains it
      // and then replace it with the value
      for (auto it = this->replaced_map.begin(); it != this->replaced_map.end();
           ++it) {
        if (blk.find(it->first) != std::string::npos) {
          // replace all it->first with it->second
          blk = replace(blk, it->first, it->second);
        }
      }

      /*
      if (blk.find("<br />") != std::string::npos) {
        // replace all <br /> with \n
        std::string replaced = replace(blk, "<br />", "\n");
        std::ofstream testfile(test.c_str());
        testfile << replaced;
        continue;
      }
      */

      if (this->test_cases_used(blk)) {
        // updated test cases by codeforces (line-separated)
        // <div class="test-example-line test-example-line-odd
        // test-example-line-3" style="">BAABA</div>
        std::string ansblk = this->new_parse_tests(blk);
        std::ofstream testfile(test.c_str());

        // DEBUG
        // std::cout << ansblk << std::endl;

        testfile << ansblk;
        continue;
      }

      // need to re-check the <pre> block
      std::ofstream testfile(test.c_str());
      // if preblk[spre] == '\n' then spre += 1;
      if (blk[0] == '\n')
        blk = blk.substr(1, blk.size() - 1);

      // DEBUG
      // std::cout << blk << std::endl;
      testfile << blk;
    } else {
      throw "parse Error";
    }
  }
  return ntests;
};

bool Chainsaw::test_cases_used(std::string sinpreblk) {
  if (sinpreblk.find("test-example-line") != std::string::npos) {
    return true;
  }
  return false;
}

std::string Chainsaw::new_parse_tests(std::string newpreblk) {
  std::string ret = "";
  int start = 0;
  int end_div = 0;

  while (true) {
    end_div = newpreblk.find("</div>", end_div + 6);
    start = newpreblk.rfind("\">", end_div);
    ret += newpreblk.substr(start + 2, end_div - start - 2);
    ret += '\n';
    if (end_div + 7 > newpreblk.size())
      break;
  }

  return ret;
}

std::string Chainsaw::replace(std::string preblk, std::string oldstr,
                              std::string newstr) {
  size_t pos = 0;
  while ((pos = preblk.find(oldstr, pos)) != std::string::npos) {
    preblk.replace(pos, oldstr.length(), newstr);
    pos += newstr.length();
  }
  return preblk;
}

/**
 * main() arguments:
 * cf,  iscontest,  conn
 */
int main(int argc, char *argv[]) {
  std::string conn = argv[1]; // contest number
  std::vector<std::string> vprob = {};
  std::string conturl = CFURL + conn;

  if (argc > 2) {
    for (int i = 2; i < argc; i++) {
      vprob.push_back(argv[i]);
    }
    // Multithreading
    std::vector<std::string> prbnames = vprob;
    int nprobs = prbnames.size();
    pthread_t threadpool[nprobs]; // thread pool

    for (int i = 0; i < nprobs; i++) {
      std::string url = conturl + "/problem/" + prbnames[i];
      void *cfurl = static_cast<void *>(new std::string(url));
      int result = pthread_create(&threadpool[i], NULL, curlPre, cfurl);
      if (result != 0)
        std::cerr << "Error on creating thread " << i << std::endl;
    }

    for (int i = 0; i < nprobs; i++) {
      // Execute threads
      pthread_join(threadpool[i], NULL);
    }

    for (std::pair<std::string, std::string> pre : preblks) {
      Chainsaw *CsObject = new Chainsaw();
      CsObject->parseTests(pre.second, pre.first);
    }
  } else {
    // mapping problem number into its preblk
    CurlObj *cocont = new CurlObj(conturl);
    std::string conthtml = cocont->getData();

    Chainsaw *contCs = new Chainsaw();
    int np = contCs->parseProblems(conthtml);

    for (auto c : contCs->getProb()) {
      std::cout << c << std::endl;
    }
  }
}
