#include <iostream>
#include "utils.h"

using namespace std;

string convert_int_to_ipv4(uint32_t ipAddress) {
    uint32_t octet1 = (ipAddress >> 24) & 0xFF;
    uint32_t octet2 = (ipAddress >> 16) & 0xFF;
    uint32_t octet3 = (ipAddress >> 8) & 0xFF;
    uint32_t octet4 = ipAddress & 0xFF;
    return (to_string(octet1) + "." +
        to_string(octet2) + "." +
        to_string(octet3) + "." +
        to_string(octet4));
}

uint64_t convert_string_ipv4_internal(string addr) {
    stringstream s;
    string intermediate;
    stringstream check1(addr);
    int i = 0;
    while(getline(check1, intermediate, '.')) {
        s << setfill('0') << setw(2) << hex << stoi(intermediate);
        i++;
    }
    return stoul(s.str(), 0, 16);
}

uint64_t convert_string_preamble_internal(string addr) {
    // vector <string> tokens;
    stringstream s;
    // string intermediate;
    // stringstream check1(addr);

    for (uint32_t i=0; i<addr.size(); i+=2) {
        // tokens.push_back(address.substr(i, 2));
        s << setfill('0') << setw(2) << hex << stoi(addr.substr(i,2));
    }
    return stoul(s.str(), 0, 16);

    // while(getline(check1, intermediate, '.')) {
    //     s << setfill('0') << setw(2) << hex << stoi(intermediate);
    //     i++;
    // }
    // return stoul(s.str(), 0, 16);
}

int main() {

    // //--------------------------------------
    // // Exercise:1 Copy 64B into uchar array
    // //--------------------------------------
    // cout << "EXERCISE-1" << string (80, '-') << endl;
    // uint64_t dstAddr = 0xffffffffffff;
    // cout << "Destination Address: " << hex << dstAddr << endl;
    // 
    // unsigned char data[12];
    // memcpy(data, (char*)&dstAddr, 6);
    // print_array(data, 6);
    // dstAddr++;
    // memcpy(data, (char*)&dstAddr, 6);
    // print_array(data, 6);
    // 
    // //--------------------------------------------
    // // Exercise2: Convert string of hex into long
    // //--------------------------------------------
    // cout << "EXERCISE-2" << string (80, '-') << endl;
    // string hexStr = "aabbccddeeff11ff";
    //  uint64_t hexNum = stoul(hexStr, 0, 16);
    // cout << "hexNum: " << hex << hexNum << endl << endl;
    // hexNum++;
    // cout << "hexNum: " << hex << hexNum << endl << endl;
    //
    //--------------------------------------------
    // Exercise2a: Convert mac address into long
    //--------------------------------------------
    // cout << "EXERCISE-2" << string (80, '-') << endl;
    // string hexStr = "aa:bb:cc:dd:ee:ff";
    // hexStr.erase(remove(hexStr.begin(), hexStr.end(), ':'), hexStr.end());
    // uint64_t hexNum = stoul(hexStr, 0, 16);
    // cout << "hexNum: " << hex << hexNum << endl << endl;
    // hexNum++;
    // cout << "hexNum: " << hex << hexNum << endl << endl;
    // 
    // //---------------------------------------
    // // Exercise3: Convert IPv4 to hex string
    // //---------------------------------------
    // cout << "EXERCISE-3" << string (80, '-') << endl;
    // string ipaddr = "255.255.255.250";
    // uint64_t ipinternal = convert_string_ipv4_internal(ipaddr);
    // for (uint32_t i=0; i<10; i++) {
    //     ipinternal++;
    //     cout << convert_int_to_ipv4(ipinternal) << endl;
    // }

    //------------------------------------------------
    // Exercise4: memcpy string of hex and hex integer
    //------------------------------------------------
    // cout << "EXERCISE-4" << string (80, '-') << endl;
    // string hStr = "aabbccddeeff1122";
    // uint64_t hInt = 0xaabbccddeeff1122;
    // cout << "Hex String: " << hStr << endl;
    // cout << "Hex Number: " << hex << hInt << endl;

    // unsigned char cpStr[16];

    // memcpy(cpStr, (char*)hStr.c_str(), 16);
    // print_array(cpStr, 16);

    // memcpy(cpStr, (char*)&hInt, 8);
    // print_array(cpStr, 8);

    // convert_string_to_uca(hStr, cpStr);
    // print_array(cpStr, 8);

    // cout << "-------------------------------" << endl;
    // unsigned char dataStr[16];
    // for (uint32_t i=0; i<16; i++) {
    //     // dataStr[i] = (uint16_t)i + 61;
    //     dataStr[i] = (uint16_t) i;
    // }
    // print_array(dataStr, 16);

    // unsigned char hhStr[16];
    // uint64_t hhInt = 0xaabbccdd;
    // memcpy(hhStr, (char*)&hhInt, 4);
    // print_array(hhStr, 4);

    // unsigned char ssStr[16];
    // string ssInt = "abcd";
    // memcpy(ssStr, (char*)ssInt.c_str(), 4);
    // print_array(ssStr, 4);

    return 0;
}
