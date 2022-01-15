#include <iostream>
#include <cstring>
#include <cstdint>
#include <sstream>
#include <unistd.h>
#include <iomanip>
using namespace std;

#define CEA_FORMATTED_HDR_LEN 80
string cea_formatted_hdr(string s) {
    stringstream ss;
    ss.setf(ios_base::left);
    ss << string(3,'-') << "{ ";
    ss << s << " }" << string((CEA_FORMATTED_HDR_LEN-(7+s.length())),'-');
    ss << endl;
    return ss.str();
}

void printPkt(unsigned char *data, uint32_t len) {
    ostringstream buf("");
    buf.setf(ios::hex, ios::basefield);
    buf.setf(ios_base::left);
    buf << endl;
    buf << cea_formatted_hdr("Base Packet");
    
    for (uint32_t idx=0; idx<len; idx++) {
        buf << setw(2) << right << setfill('0')<< hex << (uint16_t) data[idx] << " ";
        if (idx%8==7) buf << " ";
        if (idx%16==15) buf  << "(" << dec << (idx+1) << ")" << endl;
    }
    buf << endl << endl;

    cout << buf.str();
}

void *cea_memcpy_rev (void *dest, const void *src, size_t len) {
    char *d = (char*)dest;
    const char *s = (char*)src;
    int i = 0;
    if (len==0) return dest;
    for (i=len; i>=0; i--) {
        *d++ = s[i];
    }
    return dest;
}

int main() {
    unsigned char buf[8];
    uint64_t x = 0x12345678aabbccdd;
    // memcpy(buf, &x, 8);
    // printPkt(buf,8);

    for(uint32_t a=0; a<8; a++) {
        bzero(buf,8);
        cea_memcpy_rev(buf, &x, a);
        printPkt(buf,8);
    }
    return 0;
}
