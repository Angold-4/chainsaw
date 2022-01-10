//===----------------------------------------------------------------------===//
//
//                         chainsaw
//
// parsesubmit.cpp
//
// Identification: src/parsesubmit.cpp
//
//
// Last Modified : 2022.1.10 Jiawei Wang
//
// Copyright (c) 2022 Angold-4
//
//===----------------------------------------------------------------------===//

#include <string>
#include <iostream>
#include <curl/curl.h>
#include "cf.hpp"

using namespace::std;

/**
 * main() recieve username as input
 * return verdict, contestId, index, name, passedTestCount, timeConsumedMillis, memoryConsumedBytes
 */
int main(int argc, char* argv[]) {
    // string name = "AngoldW";
    string surl = "https://codeforces.com/api/user.status?handle=AngoldW&from=1&count=1";

    // Create curl object
    CurlObj* statCurl = new CurlObj(surl);
    string shtml = statCurl->getData();

    if (shtml.find("WRONG_ANSWER") != string::npos) {
	cout << "WRONG_ANSWER" << endl;
    } else {
	cout << "OK" << endl;
    }

    // Make as clear as possible (the string isn't so long)
    

    // contestId
    int ctis = shtml.find("contestId");
    int ctil = shtml.find(",", ctis);
    ctis += 11;
    cout << shtml.substr(ctis, ctil - ctis) << " ";

    // index
    int idxs = shtml.find("index");
    int idxl = shtml.find("\",", idxs);
    idxs += 8;
    cout << shtml.substr(idxs, idxl - idxs) << " ";

    // name
    int nams = shtml.find("name");
    int naml = shtml.find("\",", nams);
    nams += 7;
    string pname = shtml.substr(nams, naml - nams);
    for (char &c : pname) if (c == ' ') c = '_';
    cout << pname << " ";

    // passedTestCount
    int ptcs  = shtml.find("passedTestCount");
    int ptcl = shtml.find(",", ptcs);
    ptcs += 17;
    cout << shtml.substr(ptcs, ptcl-ptcs) << " ";

    // timeConsumedMillis
    int tcms = shtml.find("timeConsumedMillis");
    int tcml = shtml.find(",", tcms);
    tcms += 20;
    cout << shtml.substr(tcms, tcml-tcms) << " ";

    // memoryConsumedBytes
    int mcbs = shtml.find("memoryConsumedBytes");
    int mcbl = shtml.find("}", mcbs);
    mcbs += 21;
    cout << shtml.substr(mcbs, mcbl - mcbs);

}
