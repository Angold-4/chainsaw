//===----------------------------------------------------------------------===//
//
//                         chainsaw
//
// parseuser.cpp
//
// Identification: src/parseuser.cpp
//
//
// Last Modified : 2022.1.10 Jiawei Wang
//
// Copyright (c) 2022 Angold-4
//
//===----------------------------------------------------------------------===//


#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

using namespace::std;

string readFileIntoString(const string& path) {
    ifstream input_file(path.c_str());
    if (!input_file.is_open()) {
        cerr << "Could not open the file - '"
             << path << "'" << endl;
        exit(EXIT_FAILURE);
    }
    return string((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
}

int main() {
    string filename = "/Users/Angold4/Library/Chainsaw/temp.txt";

    string html = readFileIntoString(filename);

    string userkey = "<li><a href=\"/blog/";
    string endkey =  "\">Blog<";

    int f = html.find(userkey) + 19;
    int l = html.find(endkey);

    cout << html.substr(f, l-f) << endl;
}

