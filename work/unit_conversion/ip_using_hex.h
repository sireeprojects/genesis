#ifndef IP_USING_HEX_H
#define IP_USING_HEX_H

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <algorithm>
using namespace std;

// get IP in string format
// break into 4 parts
// convert each part into int
// convert each part into hex
// concatenate all parts as hex using masks
// increment/decrement hex
// break hex into 4 parts using masks
// convert each part into int and concatenate as a string

#define DELIMITER '.'
void tokenize(string str, vector<uint32_t> &token_v){
    size_t start = str.find_first_not_of(DELIMITER);
    size_t end = start;

    while (start != std::string::npos){
        end = str.find(DELIMITER, start);
        token_v.push_back(atoi(str.substr(start, end-start).c_str()));
        start = str.find_first_not_of(DELIMITER, end);
    }
    reverse(token_v.begin(), token_v.end());
}

uint32_t ip2hex(vector<uint32_t> itokens) {
    uint32_t iphex = 0;
    for (uint32_t i=0; i<4; i++) {
        iphex = iphex |  (itokens[i] << (8*i));
    }
    return iphex;
}

string hex2ipstr(uint32_t iphex) {
    stringstream result;
    uint32_t tmp = 0;

    for (int32_t i=3; i>-1; i--) {
        tmp = (0xff000000 & iphex) >> (24);
        iphex = iphex << 8;
        result << dec << tmp;
        if (i !=0) {
            result << ".";
        }
    }
    return result.str();
}

void test_hex() {
    // string ip = "192.168.1.0";
    string ip = "255.255.254.254";

    vector<uint32_t> int_tokens;
    tokenize(ip, int_tokens);
    uint32_t test_ip = ip2hex(int_tokens);

    for(auto i=0; i<10; i++) {
        cout << "test: " << hex2ipstr(test_ip) << endl;
        test_ip = test_ip + 1; // increment ip
    }
}

#endif // IP_USING_HEX_H
