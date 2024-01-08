#include <iostream>
#include <cstring>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <bits/stdc++.h>
using namespace std;


void print_cdata (unsigned char* tmp, int len) {
    stringstream s;
    s.str("");
    uint32_t idx = 0;

    for (int x=0; x<len/16; x++) {
        for (int y=0; y<16; y++) {
            s << noshowbase << setw(2) << setfill('0')
              << hex << uint16_t(tmp[idx]) << " ";
            idx++;
        }
        s << endl;
    }
    for (int x=idx; x<len; x++) {
       s<<noshowbase<<setw(2)<<setfill('0') <<hex<<uint16_t(tmp[idx])<<" ";
       idx++;
    }
    cout << "PKT Data :" << endl << s.str()<<endl;
    fflush (stdout);
}


int GetDigitValue (char digit) {
    int asciiOffset, digitValue;
    if (digit >= 48 && digit <= 57) {
          // code for '0' through '9'
        asciiOffset = 48;
        digitValue = digit - asciiOffset;
        return digitValue;
    }
    else if (digit >= 65 && digit <= 70) {
        // digit is 'A' through 'F'
        asciiOffset = 55;
        digitValue = digit - asciiOffset;
        return digitValue;
    }
    else if (digit >= 97 && digit <= 122) {
        // code for 'a' through 'f'
        asciiOffset = 87;
        digitValue = digit - asciiOffset;
        return digitValue;
    }
    else {
        // illegal digit
    }
    return 0;
}

// assumes 2 character string with legal hex digits
unsigned char Convert(string hexNumber) {
     unsigned char aChar;
     char highOrderDig = hexNumber[0];
     char lowOrderDig  = hexNumber[1];
     int lowOrderValue = GetDigitValue(lowOrderDig); //  convert lowOrderDig to number from 0 to 15
     int highOrderValue = GetDigitValue(highOrderDig); // convert highOrderDig to number from 0 to 15
     aChar = lowOrderValue + 16 * highOrderValue;
     return aChar;
}

int main() {
    string saddr = "a1:b2:c3:d4:e5:f6";
    char addr[saddr.length()];
    memcpy(addr, saddr.c_str(), saddr.length()+1);
                                                   
    unsigned char naddr[6];
    memset(naddr, '0', 6);

    stringstream check1(saddr);

    vector <string> tokens;
    string intermediate;

    while(getline(check1, intermediate, ':')) {
        tokens.push_back(intermediate);
    }
    for (uint32_t i=0; i<6; i++) {
        naddr[i]= Convert(tokens[i]);
    }

   	char buf[256] ;
    sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x ",
        naddr[0], naddr[1], naddr[2],
        naddr[3], naddr[4], naddr[5]);

    printf("char array addr: %s\n", buf);

    unsigned char frame[64];

    memcpy(frame, naddr, 6);
    print_cdata(frame, 6);

    return 0;
}
