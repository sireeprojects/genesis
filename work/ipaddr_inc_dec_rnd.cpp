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
                    addr[3] = 0x00;
                    addr[2] -= 1;
                    if (addr[2] == (MAXIP+1)) {
                        addr[2] = 0xFF;
                        addr[1] -= 1;
                        if (addr[1] == (MAXIP+1)) {
                            addr[1] = 0xFF;
                            addr[0] -= 1;
                            if (addr[0] == (MAXIP+1)) {
                                addr[0] = 0xFF;
                                addr[3] += 0x00;
                            }
                        }
                    }
                }
                addr[3] -= 1;
                break;
                }
            case (Random): {
                unsigned char cnt;
                for(uint32_t j=0;j<4;j++) {
                    cnt=rand()%255;
                    memcpy((char*)addr+j, &cnt, 1);
                }
                break;
                }
            default:{
                break;
                }
        }
    } else {
    }
}

int main() {
    char buf[256];

    unsigned char *addr = new unsigned char[4];

    addr[0] = 0x00;
    addr[1] = 0x00;
    addr[2] = 0x00;
    addr[3] = 0x00;
    for (uint32_t cnt=0; cnt<270; cnt++) {
        sprintf(buf, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);
        cout << buf << endl;
        get_next_addr(addr, IP, Increment);
    }

    cout << endl;

    addr[0] = 0xff;
    addr[1] = 0xff;
    addr[2] = 0xff;
    addr[3] = 0xff;
    for (uint32_t cnt=0; cnt<270; cnt++) {
        sprintf(buf, "%02x.%02x.%02x.%02x", addr[0], addr[1], addr[2], addr[3]);
        cout << buf << endl;
        get_next_addr(addr, IP, Decrement);
    }

    cout << endl;

    addr[0] = 0x00;
    addr[1] = 0x00;
    addr[2] = 0x00;
    addr[3] = 0x00;
    srand(time(NULL));
    for (uint32_t cnt=0; cnt<10; cnt++) {
        get_next_addr(addr, IP, Random);
        sprintf(buf, "%02x.%02x.%02x.%02x", addr[0], addr[1], addr[2], addr[3]);
        cout << buf << endl;
    }

    delete[] addr;
}
