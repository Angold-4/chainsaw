//===----------------------------------------------------------------------===//
//
//                         chainsaw
//
// cf.hpp
//
// Identification: src/cf.hpp
//
//
// Last Modified : 2022.1.2 Jiawei Wang
//
// Copyright (c) 2022 Angold-4
//
//===----------------------------------------------------------------------===//

#pragma once

#include <curl/curl.h>
#include <fstream>
#include <stdio.h>
#include <string>
#include <iostream>



// CurlObj is the class to handle url
class CurlObj {
public:
    CurlObj (std::string url) : cfurl(url) {
	curl = curl_easy_init();
	if (!curl) throw std::string("Curl did not initialize.");
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriter);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &curlBuffer);
	curl_easy_perform(curl);
    }

    static int curlWriter(char* data, int size, int nmemb, std::string* buffer) {
	// write to the buffer
	int result = 0;
	if (buffer != NULL) {
	    buffer->append(data, size * nmemb);
	    result = size * nmemb;
	}
	return result;
    }

    std::string getData() {
	return curlBuffer;
    }

private:
    CURL* curl;
    std::string cfurl;
    std::string curlBuffer; // write to curlBuffer
};


// Chainsaw is the class to handle cf html
class Chainsaw {
public:
    Chainsaw(std::string html) : cfhtml(html) {}

    /** 
     * Chainsaw::getPreBlock()
     * 
     * Get The <pre> </pre> Block (if have multiple then all return)
     */
    std::string getPreBlock(std::string html);

    /**
     * Chainsaw::parseTests()
     *
     * Parse the pre block 
     * and then get # of tests
     * and write each tests to each file
     */
    int parseTests(std::string preblk);

private:
    std::string cfhtml;
    int ntestcase = 0;
};

