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
    }

    static int curlWriter(char* data, int size, int nmemb, std::string* buffer) {

    }

private:
    std::string cfurl;
    CURL* curl;
};

// Chainsaw is the class to handle cf html
class Chainsaw {
public:
    Chainsaw(std::string html) : cfhtml(html) {


    }

private:
    std::string cfhtml;
};


