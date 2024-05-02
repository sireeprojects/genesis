#include <iostream>
#include <stdio.h>
#include <string.h>
#define MAC_ADDR_BYTES 4
#define MAX_U8_VAL     0xFF

using namespace std;

void incr_ip_addr(unsigned char *pAddr, unsigned int max_val) {
    for (int i=MAC_ADDR_BYTES-1; i>=0 ; i--) {
        if (pAddr[i] == 0xFF) {
            pAddr[i] = 0x00;
        } else {
            pAddr[i]++;
            break;
        }
    }
}

void decr_ip_addr(unsigned char *pAddr, unsigned int max_val) {
    for (int i=MAC_ADDR_BYTES-1; i>=0 ; i--) {
        if (pAddr[i] == 0x00) {
            pAddr[i] = 0xFF;
        } else {
            pAddr[i]--;
            break;
        }
    }
}

int main(void) {

	unsigned int i;
   	char buf[256] ;
	unsigned char mac_addr[MAC_ADDR_BYTES];
	unsigned char init_mac_addr[] = {0x02, 0x01, 0xff, 0xff};
   
   	memcpy(&mac_addr[0], &init_mac_addr, sizeof(unsigned char)*MAC_ADDR_BYTES) ;

   for(i=0; i<5; i++) {
    	sprintf(buf, "%02x:%02x:%02x:%02x ", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3]);
    	printf ("%s\n", buf);
        incr_ip_addr(mac_addr, MAX_U8_VAL) ;
    	// decr_ip_addr(mac_addr, MAX_U8_VAL) ;
   }
}

