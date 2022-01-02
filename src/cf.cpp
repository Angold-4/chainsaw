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


std::string Chainsaw::getPreBlock(std::string html) {
    int fpre = html.find("<pre>");
    int lpre = html.rfind("</pre>");
    return html.substr(fpre, lpre-fpre)+"</pre>";
};

int Chainsaw::parseTests(std::string preblk) {
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
		test = "input/test" + std::to_string(ntests / 2 + 1) + ".txt";
		isTest = false;
	    } else {
		test = "input/ans" + std::to_string(ntests / 2) + ".txt";
		isTest = true;
	    } 
	    std::ofstream testfile(test.c_str());
	    std::cout << test << std::endl;
	    testfile << preblk.substr(spre, epre-spre);
	} else {
	    throw "parse Error";
	}
    }
    return ntests;
};

/*
<pre>
4 2
0 1 2 3
</pre></div><div class="output"><div class="title">Output</div><pre>
8
</pre></div><div class="input"><div class="title">Input</div><pre>
3 6
4 2 2
</pre></div><div class="output"><div class="title">Output</div><pre>
7
</pre></div><div class="input"><div class="title">Input</div><pre>
4 0
1 1 2 2
</pre></div><div class="output"><div class="title">Output</div><pre>
6
</pre>
*/


int main() {
    // Test
    std::string url = "https://codeforces.com/contest/1616/problem/H";
    CurlObj* co = new CurlObj(url);
    std::string html = co->getData();
    Chainsaw* CsObject = new Chainsaw(html);
    std::string preblk = CsObject->getPreBlock(html);
    std::cout << preblk << std::endl;
    CsObject->parseTests(preblk);
}

