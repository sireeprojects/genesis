#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <iomanip>

#define FSIZE 6

using namespace std;


int main() {

    string macaddr = "00:00:00:00:00:00";
    // string macaddr = "ff:ff:ff:ff:ff:ff";
    // cout << macaddr << endl;

    macaddr.erase(remove(macaddr.begin(), macaddr.end(), ':'), macaddr.end());
    // cout << macaddr << endl;

    uint64_t intmac = stol(macaddr, 0, 16);
    // cout << hex << intmac << endl;

    unsigned char mac[6];

    // cout << hex << setfill('0') << setw(12) << intmac << endl;

    for(int i=0; i<10; i++) {
        // cout << hex << setfill('0') << setw(12) << intmac << endl;
        memcpy(mac, &intmac, 6);
        for(int j=0; j<FSIZE; j++) {
            cout <<hex<<setw(2)<<setfill('0')<< (unsigned short)mac[j];
        }
        intmac -= 256;
        // long im = strtol(mac,NULL,16);
        // cout << hex << im;
        cout << endl << endl;
    }


    // for(int i=0; i<4; i++) {
    //     cout << vca[i] << endl;
    // }

    return 0;
}
