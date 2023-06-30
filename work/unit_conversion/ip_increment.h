#ifndef IP_INCREMENT_H
#define IP_INCREMENT_H

#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
using namespace std;

#define DELIMITER '.'
#define ROLLOVER 0xff

void tokenize(string str, vector<string> &token_v){
    size_t start = str.find_first_not_of(DELIMITER);
    size_t end = start;

    while (start != std::string::npos){
        end = str.find(DELIMITER, start);
        token_v.push_back(str.substr(start, end-start));
        start = str.find_first_not_of(DELIMITER, end);
    }
    reverse(token_v.begin(), token_v.end());
}

string stringize(vector<uint32_t> &token_v) {
    string s = to_string(token_v[3]) + "."
               + to_string(token_v[2]) + "."
               + to_string(token_v[1]) + "."
               + to_string(token_v[0]);
    return s;
}

string increment_ip(vector<uint32_t> &token_v) {
    if (token_v[0] == ROLLOVER and token_v[1] != ROLLOVER and token_v[2] != ROLLOVER and token_v[3] != ROLLOVER) {
        token_v[0] = 0;
        token_v[1]++;
    } else if (token_v[0] == ROLLOVER and token_v[1] == ROLLOVER and token_v[2] != ROLLOVER and token_v[3] != ROLLOVER) {
        token_v[0] = 0;
        token_v[1] = 0;
        token_v[2]++;
    } else if (token_v[0] == ROLLOVER and token_v[1] == ROLLOVER and token_v[2] == ROLLOVER and token_v[3] != ROLLOVER) {
        token_v[0] = 0;
        token_v[1] = 0;
        token_v[2] = 0;
        token_v[3]++;
    } else if (token_v[0] == ROLLOVER and token_v[1] == ROLLOVER and token_v[2] == ROLLOVER and token_v[3] == ROLLOVER) {
        for (uint32_t i=0; i<4; i++) {
            token_v[i] = 0;
        }
    } else {
        token_v[0]++;
    }
    return stringize(token_v);
}

void ip_increment_test() {
    string ipaddress = "254.254.255.254";
    // string ipaddress = "192.168.255.255";
    // string ipaddress = "0.0.0.0";
    cout << ipaddress << endl;

    vector<string> tokens;
    tokenize(ipaddress, tokens);

    vector<uint32_t> int_tokens(4);
    uint32_t idx = 0;
    for (auto i : tokens) {
        int_tokens[idx] = atoi(i.c_str());
        idx++;
    }

    for (uint32_t cnt=0; cnt<20; cnt++) {
        cout << increment_ip(int_tokens) << endl;
    }
}

#endif // IP_INCREMENT_H
