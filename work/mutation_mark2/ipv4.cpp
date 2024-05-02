#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <sstream>
#include <vector>
#include <iomanip>
#define IPV4_SIZE 4

using namespace std;

void convert_string_to_uca(string address, unsigned char *op);
unsigned char convert_char_to_int(string hexNumber);
int convert_nibble_to_int(char digit);
void print_cdata (unsigned char* tmp, int len);

int main() {
    string ipv4addr = "192.168.0.1";
	unsigned char patt_array[IPV4_SIZE];
    unsigned char pf[100];
    memset(pf, 0, 100);

    convert_string_to_uca(ipv4addr, patt_array);
    print_cdata(patt_array, IPV4_SIZE);
    memcpy(pf, patt_array, IPV4_SIZE);
    print_cdata(pf, 100);

    return 0;
}

void convert_string_to_uca(string address, unsigned char *op) {
    stringstream check1(address);
    string intermediate;
    int i = 0;

    while(getline(check1, intermediate, '.')) {
        op[i] = stoi(intermediate);
        i++;
    }
}

int convert_nibble_to_int (char digit) {
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
    cout << s.str()<<endl;
    fflush (stdout);
}
