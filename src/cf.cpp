//===----------------------------------------------------------------------===//
//
//                         chainsaw
//
// cf.cpp
//
// Identification: src/cf.cpp
//
//
// Last Modified : 2022.1.4 Jiawei Wang
//
// Copyright (c) 2022 Angold-4
//
//===----------------------------------------------------------------------===//


#include "./cf.hpp"


static int curlWriter(char* data, int size, int nmemb, std::string* buffer) {
    // write to the buffer
    int result = 0;
    if (buffer != NULL) {
	buffer->append(data, size * nmemb);
	result = size * nmemb;
    }
    return result;
}

std::unordered_map<std::string, std::string> preblks; // all preblks for multithreading

void *curlPre(void* url) {
    // For multithreading
    CURL* curl;
    std::string curlBuffer;

    std::string *sp = static_cast<std::string*>(url);
    std::string URL = *sp;


    delete sp;

    curl = curl_easy_init();
    if (!curl) throw std::string("Curl did not initialize.");
    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriter);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlBuffer);
    curl_easy_perform(curl);

    int fpre = curlBuffer.find("<pre>");
    int lpre = curlBuffer.rfind("</pre>"); // speed up TODO

    int pl = URL.rfind('/'); // problem number in the last
    std::string prbname = URL.substr(pl+1, URL.size() - pl);
    preblks[prbname] = curlBuffer.substr(fpre, lpre-fpre) + "</pre>"; // set the global variable
    pthread_exit(NULL);
}



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
    std::string iscontest = argv[1];  // is contest ? 
    std::string conn = argv[2];       // contest number
    
    // mapping problem number into its preblk

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
	// Multithreading
	std::vector<std::string> prbnames = contCs->getProb();
	int nprobs = prbnames.size();
	pthread_t threadpool[nprobs];        // thread pool
	for (int i = 0; i < nprobs; i++) {

	    std::string url = conturl + "/problem/" + prbnames[i];
	    void *cfurl = static_cast<void*>(new std::string(url));
	    int result = pthread_create(&threadpool[i], NULL, curlPre, cfurl);
	    if (result != 0) std::cerr << "Error on creating thread " << i << std::endl;
	}


	for (int i = 0; i < nprobs; i++) {
	    // Execute threads
	    pthread_join(threadpool[i], NULL);
	}

	for (std::pair<std::string, std::string> pre : preblks) {
	    Chainsaw* CsObject = new Chainsaw();
	    CsObject->parseTests(pre.second, pre.first);
	}

	/*
	// Generate all sample tests
	for (std::string s : contCs->getProb()) {
	    std::string url = conturl + "/problem/" + s;
	    CurlObj* co = new CurlObj(url);
	    std::string html = co->getData(); // html is the preblk

	    Chainsaw* CsObject = new Chainsaw();
	    CsObject->parseTests(html, s);
	}
	*/
    }
}

