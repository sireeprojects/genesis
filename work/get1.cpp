#include <iostream>
#include <cstdint>
#include <string>

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
    uint32_t maxidx = (type==IP)? 3 : 5;

    switch (action) {
        case (Increment): {
            if (addr[maxidx]==0x04) {
                addr[maxidx] = 0x00;
                addr[maxidx-1] += 1;
                for (uint32_t i=maxidx-1; i>0; i--) {
                    if (addr[i] > 0x04) {
                        addr[i] = 0;
                        addr[i-1] += 1;
                    }
                }
            } else
                addr[maxidx] += 1;
            break;
            }
        case (Decrement): {
            if (addr[maxidx]==0x00) {
                addr[maxidx] = 0xff;
                addr[maxidx-1] -= 1;
                for (uint32_t i=maxidx-1; i>0; i--) {
                    if (addr[i] > 0x00) {
                        addr[i] = 0x00;
                        addr[i-1] -= 1;
                    }
                }
            } else
                addr[maxidx] -= 1;
            break;
            }
        default:{
            break;
            }
    }
}

int main() {
    char buf[256];

    unsigned char *addr = new unsigned char[4];

    // addr[0] = 0x00;
    // addr[1] = 0x00;
    // addr[2] = 0x00;
    // addr[3] = 0x00;

    // for (uint32_t cnt=0; cnt<10; cnt++) {
    //     sprintf(buf, "%02x.%02x.%02x.%02x", addr[0], addr[1], addr[2], addr[3]); cout << buf << endl;
    //     if (cnt%5==4) cout << endl;
    //     get_next_addr(addr, IP, Increment);
    //     // get_next_addr(addr, IP, Decrement);
    // }

    cout << endl;

    addr[0] = 0xff;
    addr[1] = 0xff;
    addr[2] = 0xff;
    addr[3] = 0xff;
    for (uint32_t cnt=0; cnt<260; cnt++) {
        sprintf(buf, "%02x.%02x.%02x.%02x", addr[0], addr[1], addr[2], addr[3]); cout << buf << endl;
        if (cnt%5==4) cout << endl;
        get_next_addr(addr, IP, Decrement);
    }
}
