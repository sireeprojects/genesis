#include <iostream>
#include <cstring>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <bits/stdc++.h>
#define MAX_U8_VAL 0xFF
#define IPV4_ADDR_SIZE 4

using namespace std;

int convert_nibble_to_int(char digit);
unsigned char convert_char_to_int(string hexNumber);
void incr_ip_addr(unsigned char *pAddr, unsigned int max_val);
void convert_string_to_uca(string address, unsigned char *op);

int main() {
    string user_addr = "192.255.254.1";
	unsigned char addr_value[IPV4_ADDR_SIZE];

    convert_string_to_uca(user_addr, addr_value);

    for (uint32_t i=0; i<520; i++) {
   	    char buf[256] ;
        // sprintf(buf, "%02x:%02x:%02x:%02x",
        sprintf(buf, "%d:%d:%d:%d",
                addr_value[0], addr_value[1],
                addr_value[2], addr_value[3]);
        incr_ip_addr(addr_value, MAX_U8_VAL);
        printf("Incremented Addr: %s\n", buf);
    }
    return 0;
}

void convert_string_to_uca(string address, unsigned char *op) {
    stringstream check1(address);
    vector <string> tokens;
    string intermediate;
    int i = 0;

    while(getline(check1, intermediate, '.')) {
        op[i] = stoi(intermediate);
        i++;
    }
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

void incr_ip_addr(unsigned char *pAddr, unsigned int max_val) {
	unsigned int MSB = IPV4_ADDR_SIZE - 1;

	if (pAddr[MSB] == MAX_U8_VAL) {
		pAddr[MSB] = 0x00;
		if (pAddr[MSB-1] == MAX_U8_VAL) {
			pAddr[MSB-1] = 0x00;
			if (pAddr[MSB-2] == MAX_U8_VAL) {
				pAddr[MSB-2] = 0x00;
				if (pAddr[MSB-3] == MAX_U8_VAL) {
					pAddr[MSB-3] = 0x00;
				} else {
					pAddr[MSB-3]++;
				}
			} else {
				pAddr[MSB-2]++;
			}
		} else {
			pAddr[MSB-1]++;
		}
	} else {
		pAddr[MSB]++;
	}
}
