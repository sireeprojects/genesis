#include <iostream>
#include <cstring>
#include <string>
#include <cstdint>
#include <iomanip>

using namespace std;

void print_cdata(unsigned char* tmp, int len);

void print_cdata(unsigned char* tmp, int len) {
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
