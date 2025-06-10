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

int main() {
    //--------------------------------------
    // Exercise:1 Copy 64B into uchar array
    //--------------------------------------
    cout << "EXERCISE-1" << string (80, '-') << endl;
    uint64_t dstAddr = 0xaabbccddeeff;
    cout << "Destination Address: " << hex << dstAddr << endl;
    
    unsigned char data[12];
    memcpy(data, (char*)&dstAddr, 6);
    print_array(data, 6);
    dstAddr++;
    memcpy(data, (char*)&dstAddr, 6);
    print_array(data, 6);
    
    //--------------------------------------------
    // Exercise2: Convert string of hex into long
    //--------------------------------------------
    cout << "EXERCISE-2" << string (80, '-') << endl;
    string hexStr = "aabbccddeeff1122";
    unsigned long long hexNum = stoul(hexStr, 0, 16);
    cout << "hexNum: " << hex << hexNum << endl << endl;
    
    //---------------------------------------
    // Exercise3: Convert IPv4 to hex string
    //---------------------------------------
    cout << "EXERCISE-3" << string (80, '-') << endl;
    string ipaddr = "255.255.255.250";
    uint64_t ipinternal = convert_string_ipv4_internal(ipaddr);
    for (uint32_t i=0; i<10; i++) {
        ipinternal++;
        cout << convert_int_to_ipv4(ipinternal) << endl;
    }
    return 0;
}
