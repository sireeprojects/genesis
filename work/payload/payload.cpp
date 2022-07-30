#include "payload.h"
using namespace std;

#define ONE_MB (1024*1024)
unsigned char *buf;
uint32_t hdr_size = 14;
uint32_t fcs_size = 4;
uint32_t frm_size = 64;
uint32_t pl_size = 0;

void randomize_frame_buf() {
    random_device rd; // obtain a random number from hardware
    mt19937 gen(rd()); // seed the generator
    uniform_int_distribution<> distr(0, 255); // define the range
    for(int idx=0; idx<ONE_MB; idx++) {
        buf[idx] =  static_cast<char>(distr(gen));
    }
}

void create_frame_buffer() {
    buf = new unsigned char[ONE_MB];
}

int main() {
    create_frame_buffer();
    randomize_frame_buf();

    // find payload size
    pl_size = frm_size - (hdr_size + fcs_size);

    cout << cea_formatted_hdr("Parameters");
    cealog << toc(30, "Configured Frame Size") << frm_size << endl;
    cealog << toc(30, "Header size") << hdr_size << endl;
    cealog << toc(30, "FCS size") << fcs_size << endl;
    cealog << toc(30, "Payload size") << pl_size << endl;;
    
    print_frame(buf, frm_size);

    delete(buf);
    return 0;
}

