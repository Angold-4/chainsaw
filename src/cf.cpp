//===----------------------------------------------------------------------===//
//
//                         chainsaw
//
// cf.cpp
//
// Identification: src/cf.cpp
//
//
// Last Modified : 2022.1.2 Jiawei Wang
//
// Copyright (c) 2022 Angold-4
//
//===----------------------------------------------------------------------===//


#include "./cf.hpp"


int Chainsaw::parseProblems(std::string html) {
    int foption = html.find("<option value=\"A\" >");
    int lselect = html.find("</select>", foption);
    std::string optblk = html.substr(foption, lselect-foption);

    int count = 0;
    size_t found = 0;
    size_t probindex = 0;
    size_t eindex = 0;
    // store the name of problems
    this->probnames = {};
    while (true) {
	found = optblk.find("\n", found+1);
	if (found == std::string::npos) break;
	probindex = optblk.find("e=", probindex+1);
	eindex = optblk.find("\" >", probindex);
	probindex+=3; // value="B" >

	std::string pname = optblk.substr(probindex, eindex-probindex);
	this->probnames.push_back(pname);
	count++;
    }
    return count;
}


std::string Chainsaw::getPreBlock(std::string html) {
    int fpre = html.find("<pre>");
    int lpre = html.rfind("</pre>");
    return html.substr(fpre, lpre-fpre)+"</pre>";
};


int Chainsaw::parseTests(std::string preblk, std::string prob) {
    int ntests = 0;
    size_t spre = 0; // start pre
    size_t epre = 0; // end pre
    bool isTest = true;

    while (true) {
	spre = preblk.find("<pr", spre);
	if (spre == std::string::npos) break;
	spre += 6;
	epre = preblk.find("</pr", spre);

	if (spre < epre) {
	    ntests++;
	    std::string test;
	    // ofstream never create dir
	    if (isTest) {
		test = "sample/"+prob+"/input" + std::to_string(ntests / 2 + 1) + ".txt";
		isTest = false;
	    } else {
		test = "sample/"+prob+"/output" + std::to_string(ntests / 2) + ".txt";
		isTest = true;
	    } 
	    std::ofstream testfile(test.c_str());
	    testfile << preblk.substr(spre, epre-spre);
	} else {
	    throw "parse Error";
	}
    }
    return ntests;
};


/**
 * main() arguments:
 * cf,  iscontest,  conn
 */
int main(int argc, char* argv[]) {
    std::string iscontest = argv[1]; // is contest ? 
    std::string conn = argv[2];      // contest number
    
    std::string conturl = CFURL + conn;
    CurlObj* cocont = new CurlObj(conturl);
    std::string conthtml = cocont->getData();
    Chainsaw *contCs = new Chainsaw();
    int np = contCs->parseProblems(conthtml);

    if (iscontest == "1") {
	for (auto c : contCs->getProb()) {
	    std::cout << c << std::endl;
	}
    } else {
	// Generate all sample tests
	for (std::string s : contCs->getProb()) {
	    std::string url = conturl + "/problem/" + s;
	    CurlObj* co = new CurlObj(url);
	    std::string html = co->getData();
	    Chainsaw* CsObject = new Chainsaw();
	    std::string preblk = CsObject->getPreBlock(html);
	    CsObject->parseTests(preblk, s);
	}
    }
}

