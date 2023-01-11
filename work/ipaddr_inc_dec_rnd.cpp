#include <iostream>
#include <cstdint>
#include <string>
#include <cstring>

using namespace std;

enum cea_addr_type {
    IP,
    MAC
};

enum cea_addr_gen_type {
    Random,
    Increment,
    Decrement
};

void get_next_addr(unsigned char *addr, cea_addr_type type, cea_addr_gen_type action) {
    unsigned char MAXIP;

    if (type==IP) {
        if (action == Increment)
           MAXIP = 0xFF;
        else if (action == Decrement)
           MAXIP = 0x00;

        switch (action) {
            case (Increment): {
                if (addr[3] == MAXIP) {
                    addr[3] = 0x00;
                    addr[2] += 1;
                    if (addr[2] == (MAXIP+1)) {
                        addr[2] = 0x00;
                        addr[1] += 1;
                        if (addr[1] == (MAXIP+1)) {
                            addr[1] = 0x00;
                            addr[0] += 1;
                            if (addr[0] == (MAXIP+1)) {
                                addr[0] = 0x00;
                                addr[3] += 0;
                            }
                        }
                    }
                } else 
                    addr[3] += 1;
                break;
                }
            case (Decrement): {
                if (addr[3] == MAXIP) {
                    addr[3] = 0;
                    addr[2] -= 1;
                    if (addr[2] == (MAXIP+1)) {
                        addr[2] = 0xFF;
                        addr[1] -= 1;
                        if (addr[1] == (MAXIP+1)) {
                            addr[1] = 0xFF;
                            addr[0] -= 1;
                            if (addr[0] == (MAXIP+1)) {
                                addr[0] = 0xFF;
                                addr[3] = 0;
                            }
                        }
                    }
                }
                addr[3] -= 1;
                break;
                }
            case (Random): {
                unsigned char cnt;
                for(uint32_t pos=0; pos<4; pos++) {
                    cnt=rand()%255;
                    memcpy((char*)addr+pos, &cnt, 1);
                }
                break;
                }
            default:{
                break;
                }
        }
    } else if (type==MAC) {
        if (action == Increment)
           MAXIP = 0xFF;
        else if (action == Decrement)
           MAXIP = 0x00;

        switch (action) {
            case (Increment): {
                if (addr[5] == MAXIP) {
                    addr[5] = 0x00;
                    addr[4] += 1;
                    if (addr[4] == (MAXIP+1)) {
                        addr[4] = 0x00;
                        addr[3] += 1;
                        if (addr[3] == (MAXIP+1)) {
                            addr[3] = 0x00;
                            addr[2] += 1;
                            if (addr[2] == (MAXIP+1)) {
                                addr[2] = 0x00;
                                addr[1] += 1;
                                if (addr[1] == (MAXIP+1)) {
                                    addr[1] = 0x00;
                                    addr[0] += 1;
                                    if (addr[0] == (MAXIP+1)) {
                                        addr[0] = 0x00;
                                        addr[5] += 0;
                                    }
                                }
                            }
                        }
                    }
                } else 
                    addr[5] += 1;
                break;
                }
            case (Decrement): {
                if (addr[5] == MAXIP) {
                    addr[5] = 0;
                    addr[4] -= 1;
                    if (addr[4] == (MAXIP+1)) {
                        addr[4] = 0xFF;
                        addr[3] -= 1;
                        if (addr[3] == (MAXIP+1)) {
                            addr[3] = 0xFF;
                            addr[2] -= 1;
                            if (addr[2] == (MAXIP+1)) {
                                addr[2] = 0xFF;
                                addr[1] -= 1;
                                if (addr[1] == (MAXIP+1)) {
                                    addr[1] = 0xFF;
                                    addr[0] -= 1;
                                    if (addr[0] == (MAXIP+1)) {
                                        addr[0] = 0xFF;
                                        addr[5] = 0;
                                    }
                                }
                            }
                        }
                    }
                }
                addr[5] -= 1;
                break;
                }
            case (Random): {
                unsigned char cnt;
                for(uint32_t pos=0; pos<6; pos++) {
                    cnt=rand()%255;
                    memcpy((char*)addr+pos, &cnt, 1);
                }
                break;
                }
            default:{
                break;
                }
        }
    }
}

int main() {
    char buf[256];

    unsigned char *ipaddr = new unsigned char[4];

    ipaddr[0] = 0x00;
    ipaddr[1] = 0x00;
    ipaddr[2] = 0x00;
    ipaddr[3] = 0x00;
    for (uint32_t cnt=0; cnt<270; cnt++) {
        sprintf(buf, "%d.%d.%d.%d", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
        cout << buf << endl;
        get_next_addr(ipaddr, IP, Increment);
    }

    cout << endl;

    ipaddr[0] = 0xff;
    ipaddr[1] = 0xff;
    ipaddr[2] = 0xff;
    ipaddr[3] = 0xff;
    for (uint32_t cnt=0; cnt<270; cnt++) {
        sprintf(buf, "%02x.%02x.%02x.%02x", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
        cout << buf << endl;
        get_next_addr(ipaddr, IP, Decrement);
    }

    cout << endl;

    ipaddr[0] = 0x00;
    ipaddr[1] = 0x00;
    ipaddr[2] = 0x00;
    ipaddr[3] = 0x00;
    srand(time(NULL));
    for (uint32_t cnt=0; cnt<10; cnt++) {
        get_next_addr(ipaddr, IP, Random);
        sprintf(buf, "%02x.%02x.%02x.%02x", ipaddr[0], ipaddr[1], ipaddr[2], ipaddr[3]);
        cout << buf << endl;
    }

    delete[] ipaddr;

    unsigned char *macaddr = new unsigned char[6];

    macaddr[0] = 0x00;
    macaddr[1] = 0x00;
    macaddr[2] = 0x00;
    macaddr[3] = 0x00;
    macaddr[4] = 0x00;
    macaddr[5] = 0x00;
    for (uint32_t cnt=0; cnt<270; cnt++) {
        sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
        cout << buf << endl;
        get_next_addr(macaddr, MAC, Increment);
    }

    cout << endl;
    
    macaddr[0] = 0xff;
    macaddr[1] = 0xff;
    macaddr[2] = 0xff;
    macaddr[3] = 0xff;
    macaddr[4] = 0xff;
    macaddr[5] = 0xff;
    for (uint32_t cnt=0; cnt<270; cnt++) {
        sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
        cout << buf << endl;
        get_next_addr(macaddr, MAC, Decrement);
    }
    
    cout << endl;
    
    macaddr[0] = 0x00;
    macaddr[1] = 0x00;
    macaddr[2] = 0x00;
    macaddr[3] = 0x00;
    macaddr[4] = 0x00;
    macaddr[5] = 0x00;
    srand(time(NULL));
    for (uint32_t cnt=0; cnt<10; cnt++) {
        get_next_addr(macaddr, MAC, Random);
        sprintf(buf, "%02x:%02x:%02x:%02x:%02x:%02x", macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
        cout << buf << endl;
    }
    
    delete[] macaddr;
}
