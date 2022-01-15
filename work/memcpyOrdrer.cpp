#include <iostream>
#include <cstring>
#include <cstdint>
#include <sstream>
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

int main() {
    unsigned char buf[8];
    uint16_t x = 0x1234;
    memcpy(buf, &x, 2);
    printPkt(buf,8);
    return 0;
}
