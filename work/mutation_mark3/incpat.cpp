#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#define FSIZE 6
using namespace std;

unsigned char frame[6];

void printfrm() {
    for(int j=0; j<FSIZE; j++)
        cout <<hex<<setw(2)<<setfill('0')
            <<(unsigned short)frame[j] << " ";
}

int main() {

    string macaddr = "00:00:00:00:00:00";

    // step:1
    macaddr.erase(remove(macaddr.begin(), macaddr.end(), ':'), macaddr.end());

    // step:2
    uint64_t intmac = stol(macaddr, 0, 16);

    for(int i=0; i<10; i++) {
        memcpy(frame, &intmac, FSIZE); // final copy to frame
        printfrm();
        cout << endl;
        intmac += 255; // mutation
    }

    return 0;
}
