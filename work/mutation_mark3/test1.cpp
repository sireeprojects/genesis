#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <iomanip>

using namespace std;

int main() {

    uint64_t ival = 0xabcdef12;
    cout << hex <<ival << endl;

    char dst[8];

    memcpy(dst, (char*)&ival, 8);

    for(int i=0; i<8; i++) {
        cout << hex << (uint16_t) dst[i]<< endl;
    }


    // string mac = "123456";
    string mac = "aabbccdd1234ff00";
    uint64_t imac = std::strtoll(mac, 0, 16);
    cout << hex << imac<< endl;

    return 0;
}
