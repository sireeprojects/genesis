#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <cstring>
#include <sstream>
#include <cstdint>
#include <iomanip>
#include <algorithm>

using namespace std;

void print_array (unsigned char* tmp, int len) {
    stringstream s;
    s.str("");
    uint32_t idx = 0;

    for (int x=0; x<len/16; x++) {
        for (int y=0; y<16; y++) {
            s << noshowbase << setw(2) << setfill('0')
              << hex << uint16_t(tmp[idx]) << " ";
            if (y == 7 )
                s << " ";
            idx++;
        }
        s << endl;
    }
    int spacer = 0;
    for (int x=idx; x<len; x++) {
       s<<noshowbase<<setw(2)<<setfill('0') <<hex<<uint16_t(tmp[idx])<<" ";
       idx++;
       spacer++;
       if (spacer == 8) s << " ";
    }
    cout << "Array Data:" << endl << s.str()<<endl << endl;
    fflush (stdout);
}

int convert_nibble_to_int (char digit) {
    int asciiOffset, digitValue;
    if (digit >= 48 && digit <= 57) {
        // code for '0' through '9'
        asciiOffset = 48;
        digitValue = digit - asciiOffset;
        return digitValue;
    } else if (digit >= 65 && digit <= 70) {
        // digit is 'A' through 'F'
        asciiOffset = 55;
        digitValue = digit - asciiOffset;
        return digitValue;
    } else if (digit >= 97 && digit <= 122) {
        // code for 'a' through 'f'
        asciiOffset = 87;
        digitValue = digit - asciiOffset;
        return digitValue;
    } else {
        // TODO illegal digit
    }
    return 0;
}

unsigned char convert_char_to_int(string hexNumber) {
    unsigned char aChar;
    char highOrderDig = hexNumber[0];
    char lowOrderDig  = hexNumber[1];
    int lowOrderValue = convert_nibble_to_int(lowOrderDig);
    //  convert lowOrderDig to number from 0 to 15
    int highOrderValue = convert_nibble_to_int(highOrderDig);
    // convert highOrderDig to number from 0 to 15
    aChar = lowOrderValue + 16 * highOrderValue;
    return aChar;
}

void convert_string_to_uca(string address, unsigned char *op) {
    vector <string> tokens;

    for (uint32_t i=0; i<address.size(); i+=2) {
        tokens.push_back(address.substr(i, 2));
    }
    for (uint32_t i=0; i<address.size()/2; i++) {
        op[i]= convert_char_to_int(tokens[i]);
    }
    print_array(op,4);
}

void convert_mac_to_uca(string address, unsigned char *op) {
    stringstream check1(address);
    vector <string> tokens;
    string intermediate;

    while(getline(check1, intermediate, ':')) {
        tokens.push_back(intermediate);
    }
    for (uint32_t i=0; i<6; i++) { // TODO remove hardcoded value
        op[i]= convert_char_to_int(tokens[i]);
    }
}

void convert_ipv4_to_uca(string address, unsigned char *op) {
    stringstream check1(address);
    string intermediate;
    int i = 0;

    while(getline(check1, intermediate, '.')) {
        op[i] = stoi(intermediate);
        i++;
    }
}

void convert_ipv6_to_uca(string address, unsigned char *op) {
    string ipv6addr_tmp = address;
    vector <string> tokens;

    ipv6addr_tmp.erase(remove(ipv6addr_tmp.begin(), ipv6addr_tmp.end(), ':'),
                       ipv6addr_tmp.end());

    for (size_t i=0; i<ipv6addr_tmp.size(); i+=2) {
        tokens.push_back(ipv6addr_tmp.substr(i, 2));
    }
    for (uint32_t i=0; i<16; i++) { // TODO remove hardcoded value
        op[i]= convert_char_to_int(tokens[i]);
    }
}

#endif // UTILS_H
