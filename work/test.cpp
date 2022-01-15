#include <iostream>
#include <cstdint>
#include <bitset>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <vector>
using namespace std;

#define CEA_PACKED __attribute__((packed))

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

struct CEA_PACKED cea_field {
    bool touched : 1;
    uint32_t merge : 32;
    uint64_t mask : 64;
    uint32_t id : 32;
    uint32_t len: 32;
    uint64_t value: 64;
};

cea_field fields[] = {
// Toc  Mrg  Mask Id   Len  Value    
{  0,   0,   0,   0,   4,   0x11223344              },
{  0,   6,   0,   1,   1,   4                       },
{  0,   0,   0,   2,   1,   5                       },
{  0,   0,   0,   3,   1,   0xff                    },
{  0,   0,   0,   4,   1,   0xccdd                  },
{  0,   0,   0,   5,   1,   0xee                    },
{  0,   0,   0,   6,   3,   0xff                    },
{  0,   0,   0,   7,   8,   0x12345678aabbccddull   }
};

int main() {
    bitset<32> merged;
    uint32_t offset = 0;
    unsigned char *pkt = new unsigned char[64];

    vector<uint32_t> seq = {0,1,2,3,4,5,6,7};
    uint64_t tmp =  0;
    uint64_t len = 0;
    uint64_t mlen = 0;

    for (uint32_t i=0; i<seq.size(); i++) {
        if (fields[i].merge != 0) {
            for(uint32_t x=i; x<(i+fields[i].merge); x++) {
                len = fields[x].len;
                bitset<32>t = fields[x].value;
                merged = (merged << len) | t;
                mlen += len;
            }
            tmp  = merged.to_ulong();
            memcpy(pkt+offset, (char*)&tmp, mlen/8);
            offset += mlen/8;
            i += fields[i].merge-1; // skip mergable entries
        } else {
            uint64_t tmp = fields[i].value;
            uint64_t len = fields[i].len;
            memcpy(pkt+offset, (char*)&tmp, len);
            offset += len;
        }
    }

    printPkt(pkt, 64);

    return 0;
}
