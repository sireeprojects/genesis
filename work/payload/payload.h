// TODO
// check if start and stop values are included in randomization

#include <iostream>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>

#define ONE_MB (1024*1024)

using namespace std;

enum cea_gen_type {
    Fixed,            
    Random,           
    Random_in_Range,  
    Increment,        
    Decrement,        
    Increment_Cycle,  
    Decrement_Cycle, 
    Incr_Byte,        
    Incr_Word,          
    Decr_Byte,           
    Decr_Word,            
    Repeat_Pattern,        
    Fixed_Pattern,          
    Continuous_Pkts,
    Continuous_Burst,
    Stop_After_Stream,
    Goto_Next_Stream,
    Ipg,
    Percentage,
    Pkts_Per_Sec,
    Bit_Rate
};

// use designated initializers
struct cea_gen_spec {
    uint64_t value;
    string pattern;
    uint32_t range_start;
    uint32_t range_stop;
    uint32_t range_step;
    uint32_t repeat_after;
    uint32_t step;
};

class outbuf : public streambuf {
protected:
    virtual int overflow(int c) ;
};
outbuf ob;
ostream cealog(&ob);
ofstream logfile;

int outbuf::overflow(int_type c) {
    if (c != EOF) {
    c = static_cast<char>(c);
        logfile << static_cast<char>(c);
        if (putchar(c) == EOF) {
           return EOF;
        }
    }
    return c;
}

class cea_init {
public:
    cea_init() {
        logfile.open("run.log", ofstream::out);
        if (!logfile.is_open()) {
            cout << "*** ERROR Creating logfile. Aborting..." << endl;
            exit(1);
        }
    }
    ~cea_init() { logfile.close(); }
};
cea_init init;

#define CEA_FORMATTED_HDR_LEN 80
string cea_formatted_hdr(string s) {
    stringstream ss;
    ss.setf(ios_base::left);
    ss << string(3,'-') << "{ ";
    ss << s << " }" << string((CEA_FORMATTED_HDR_LEN-(7+s.length())),'-');
    ss << endl;
    return ss.str();
}

void print_char_array(unsigned char *buf, uint32_t len, bool single=false) {
    ostringstream b("");
    b.setf(ios::hex, ios::basefield);
    b.setf(ios_base::left);
    b << endl;
    b << cea_formatted_hdr("Char Array");
    
    for (uint32_t idx=0; idx<len; idx++) {
        b << setw(2) << right << setfill('0')<< hex << 
            (uint16_t) buf[idx] << " ";
        if (!single) {
            if (idx%8==7) b << " ";
            if (idx%16==15) b  << "(" << dec << (idx+1) << ")" << endl;
        }
    }
    b << endl;
    cealog << b.str();
}

void print_uint_array(string title, uint32_t *buf, uint32_t len, bool single=false) {
    ostringstream b("");
    b.setf(ios::dec, ios::basefield);
    b.setf(ios_base::left);
    b << endl;
    b << cea_formatted_hdr(title);
    
    for (uint32_t idx=0; idx<len; idx++) {
        b << setw(3) << right << (uint16_t) buf[idx] << " ";
        if (!single) {
            if (idx%8==7) b << " ";
            if (idx%16==15) b  << "(" << dec << (idx+1) << ")" << endl;
        }
    }
    b << endl;
    cealog << b.str();
}
void fill_frame(unsigned char *buf, uint32_t start, uint32_t len, uint8_t pattern) {
    for (uint32_t idx=start; idx<(start+len); idx++) {
        buf[idx] = pattern;
    }
}

// remove trailing whitespaces from a string
string cea_rtrim(string s) {
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    return s;
}

// remove leading whitespaces from a string
string cea_ltrim(string s) {
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
    return s;
}

// remove leading and trailing whitespaces from a string
string cea_trim(string s) {
    return cea_ltrim(cea_rtrim(s));
}

string toc(uint32_t len, string msg) {
    string s = msg + string((len - cea_trim(msg).length()), '.') + " ";
    return s;
}

void randomize_char_array(unsigned char *b, uint32_t min, uint32_t max, uint32_t len) {
    random_device rd; // obtain a random number from hardware
    mt19937 gen(rd()); // seed the generator
    uniform_int_distribution<> distr(min, max); // define the range
    for(int idx=0; idx<len; idx++) {
        b[idx] = static_cast<char>(distr(gen));
    }
}

void randomize_uint_array(uint32_t *b, uint32_t min, uint32_t max, uint32_t len) {
    random_device rd; // obtain a random number from hardware
    mt19937 gen(rd()); // seed the generator
    uniform_int_distribution<> distr(min, max); // define the range
    for(int idx=0; idx<len; idx++) {
        b[idx] = distr(gen);
    }
}

string to_str(cea_gen_spec t) {
    ostringstream b("");
    b.setf(ios::dec, ios::basefield);
    b.setf(ios_base::left);
    b << endl;
    b << toc(30, "Value")        << t.value        << endl;
    b << toc(30, "Range_start")  << t.range_start  << endl;
    b << toc(30, "Range_stop")   << t.range_stop   << endl;
    b << toc(30, "Range_step")   << t.range_step   << endl;
    b << toc(30, "Repeat_after") << t.repeat_after << endl;
    b << toc(30, "Step")         << t.step         << endl;
    string name(b.str());
    return cea_trim(name);
}

string to_str(cea_gen_type t) {
    string name;
    switch(t) {
        case Fixed             : { name = "Fixed            "; break; }                                    
        case Random            : { name = "Random           "; break; }                                    
        case Random_in_Range   : { name = "Random_in_Range  "; break; }                                    
        case Increment         : { name = "Increment        "; break; }                                    
        case Decrement         : { name = "Decrement        "; break; }                                    
        case Increment_Cycle   : { name = "Increment_Cycle  "; break; }                                    
        case Decrement_Cycle   : { name = "Decrement_Cycle  "; break; }                                   
        case Incr_Byte         : { name = "Incr_Byte        "; break; }                                    
        case Incr_Word         : { name = "Incr_Word        "; break; }                                      
        case Decr_Byte         : { name = "Decr_Byte        "; break; }                                       
        case Decr_Word         : { name = "Decr_Word        "; break; }                                        
        case Repeat_Pattern    : { name = "Repeat_Pattern   "; break; }                                         
        case Fixed_Pattern     : { name = "Fixed_Pattern    "; break; }                                          
        case Continuous_Pkts   : { name = "Continuous_Pkts  "; break; }                                  
        case Continuous_Burst  : { name = "Continuous_Burst "; break; }                                   
        case Stop_After_Stream : { name = "Stop_After_Stream"; break; }                                    
        case Goto_Next_Stream  : { name = "Goto_Next_Stream "; break; }                                   
        case Ipg               : { name = "Ipg              "; break; }                      
        case Percentage        : { name = "Percentage       "; break; }                             
        case Pkts_Per_Sec      : { name = "Pkts_Per_Sec     "; break; }                               
        case Bit_Rate          : { name = "Bit_Rate         "; break; }                           
        default                : { name = "undefined        "; break; }
    }
    return cea_trim(name);
}

void *cea_memcpy_ntw_byte_order(void *dest, const void *src, size_t len) {
    if (len==0) return dest;
    char *d = (char*)dest;
    const char *s = (char*)src;
    int i = 0;
    for (i=(len-1); i>=0; i--) {
        *d++ = s[i];
    }
    return dest;
}

class payload {
public:
    // frame size
    cea_gen_type sztype;
    cea_gen_spec szspec;
    
    // payload type
    cea_gen_type ptype;
    cea_gen_spec pspec;

    void print_dimensions();
    void print_spec();
    void compute_size_start();
    void mutate();
    void reset();
    void pregenerate();
    payload();
    ~payload();
    void gen_frame();

    // mutate
    uint64_t loop_cnt;
    uint64_t pkts_per_burst;
    uint64_t burst_per_stream;

private:    
    unsigned char *buf;
    unsigned char *opbuf;
    uint32_t hdr_size;
    uint32_t fcs_size;
    uint32_t frm_size;
    uint32_t pl_size;
    uint32_t *size_idx;
    uint32_t *start_idx;

    unsigned char *inc_byte_array;
    unsigned char *dec_byte_array;
    unsigned char *inc_word_array;
    unsigned char *dec_word_array;
    uint32_t word_size;
    uint32_t nof_sizes;

    void pregen_inc_byte();
    void pregen_inc_word();
    void pregen_dec_byte();
    void pregen_dec_word();

    unsigned char *hdr;
    uint32_t hdr_len;

    uint64_t loop_control;
    uint64_t ppb_control;
    uint64_t bps_control;

    uint64_t nof_loops;
    uint64_t nof_bursts;
    uint64_t nof_pkts;
};

void payload::gen_frame() {
    switch(ptype) {
        case Incr_Byte: {
             memcpy(buf, hdr, hdr_len);
             memcpy(buf+hdr_len, inc_byte_array, (ONE_MB-hdr_len));
             print_char_array(buf, 70);
             break;
             }
        case Incr_Word: {
             memcpy(buf, hdr, hdr_len);
             memcpy(buf+hdr_len, inc_word_array, (ONE_MB-hdr_len));
             print_char_array(buf, 70);
             break;
             }
        case Decr_Byte: {
             memcpy(buf, hdr, hdr_len);
             memcpy(buf+hdr_len, dec_byte_array, (ONE_MB-hdr_len));
             print_char_array(buf, 70);
             break;
             }
        case Decr_Word: {
             memcpy(buf, hdr, hdr_len);
             memcpy(buf+hdr_len, dec_word_array, (ONE_MB-hdr_len));
             print_char_array(buf, 70);
             break;
             }
        case Fixed_Pattern: {
             memcpy(buf, hdr, hdr_len);
             memcpy(buf+hdr_len, pspec.pattern.c_str(), pspec.pattern.length());
             print_char_array(buf, 70);
             break;
             }
        case Repeat_Pattern: {
             memcpy(buf, hdr, hdr_len);
             for(uint32_t mb=0; mb<ONE_MB; mb+= pspec.pattern.length()) {
                memcpy((buf+hdr_len)+mb, pspec.pattern.c_str(), pspec.pattern.length());
             }
             print_char_array(buf, 200);
             break;
             }
        case Random: {
             randomize_char_array(buf, 0, 255, ONE_MB);
             // memcpy(buf, hdr, hdr_len); // this is done during mutation
             print_char_array(buf, 70);
             break;
             }
    }
}

// Increment byte
void payload::pregen_inc_byte() {
    for(uint32_t mb=0; mb<ONE_MB; mb+=256) {
        for(uint32_t i=0; i<256; i++) {
            inc_byte_array[i+mb] = i;
        }
    }
    // print_char_array(inc_byte_array, 1000);
}

// Increment word
void payload::pregen_inc_word() {
    for(uint32_t mb=0; mb<ONE_MB; mb+=65536*2) {
        for(uint32_t i=0; i<65536; i+=1) {
            memcpy(inc_word_array+((i*2)+mb), (unsigned char*)&i, 2);
        }
    }
    // print_char_array(inc_word_array, 145000);
}

void payload::pregen_dec_byte() {
    for(uint32_t mb=0; mb<ONE_MB; mb+=256) {
        for(uint32_t i=0; i<256; i++) {
            dec_byte_array[i+mb] = 255-i;
        }
    }
    // print_char_array(dec_byte_array, 1000);
}

void payload::pregen_dec_word() {
    uint32_t tmp = 0;
    for(uint32_t mb=0; mb<ONE_MB; mb+=65536*2) {
        for(uint32_t i=0; i<65536; i++) {
            tmp = 65535-i;
            memcpy(dec_word_array+((i*2)+mb), (unsigned char*)&tmp, 2);
        }
    }
    // print_char_array(dec_word_array, 145000);
}

void payload::pregenerate() {
    pregen_inc_byte();
    pregen_inc_word();
    pregen_dec_byte();
    pregen_dec_word();
}

payload::payload() {
    buf = new unsigned char[ONE_MB];
    opbuf = new unsigned char[ONE_MB*2];
    size_idx = new uint32_t[1024*16]; // 16K
    start_idx = new uint32_t[1024*16]; // 16K
    inc_byte_array = new unsigned char[ONE_MB]; 
    dec_byte_array = new unsigned char[ONE_MB]; 
    inc_word_array = new unsigned char[ONE_MB]; 
    dec_word_array = new unsigned char[ONE_MB]; 

    hdr_len = 24; // pre + dst + src + len
    hdr = new unsigned char[hdr_len];
    fill_frame(hdr, 0, hdr_len, 0xff); // dummy MAC header
    reset();
}

payload::~payload() {
    delete buf;
    delete size_idx;
    delete start_idx;
}

void payload::reset() {
    hdr_size = 14;
    fcs_size = 4;
    frm_size = 64;
    pl_size = 0;
    word_size = 1;

    loop_cnt = 1;
    pkts_per_burst = 1;
    burst_per_stream = 1;

    loop_control = 1;
    ppb_control = 1;
    bps_control = 1;

    nof_loops = 0;
    nof_bursts = 0;
    nof_pkts = 0;
}

void payload::compute_size_start() {
    switch (sztype) {
        case Fixed: {
             *size_idx = szspec.value;
             frm_size = szspec.value;
             nof_sizes = 1;
             print_uint_array("Fixed: Size Array", size_idx, nof_sizes);

             *start_idx = 0;
             cealog << endl << toc(30, "Start Index") << *start_idx << endl;
             break;
             }
        case Random_in_Range: {
             // determine the number of sizes allowed and allocate memeory
             nof_sizes = (szspec.range_stop - szspec.range_start);
            
             // generate and store random sizes from the given range
             randomize_uint_array(size_idx, szspec.range_start, szspec.range_stop+1, nof_sizes);
             print_uint_array("Random_in_Range: Size Array", size_idx, nof_sizes);

             for (uint32_t idx=0; idx<=nof_sizes; idx++) {
                 start_idx[idx] = idx;
             }
             print_uint_array("Random_in_Range: Start Array", start_idx, nof_sizes);
             break;
             }
        case Increment: {
             // determine the number of sizes allowed and allocate memeory
             nof_sizes = 
                 ((szspec.range_stop - szspec.range_start) / szspec.range_step) + 1;

             uint32_t idx=0;
             for (uint32_t val=szspec.range_start; val<=szspec.range_stop; val+= szspec.range_step) {
                 size_idx[idx] = val;
                 idx++;
             }
             print_uint_array("Increment: Size Array", size_idx, nof_sizes);

             *start_idx = 0;
             cealog << toc(30, "Start Index") << *start_idx << endl;
             break;
             }
        default:{
            cealog << "***ERROR: Illegal gen specification" << endl;
            abort();
        }
    }
}

void payload::print_dimensions() {
    cout << cea_formatted_hdr("Frame Dimensions");
    cealog << toc(30, "Configured Frame Size") << frm_size << endl;
    cealog << toc(30, "Header size") << hdr_size << endl;
    cealog << toc(30, "FCS size") << fcs_size << endl;
    cealog << toc(30, "Payload size") << pl_size << endl;
}

void payload::print_spec() {
    cealog << endl << cea_formatted_hdr("Payload Specification");
    cealog << toc(30, "Payload Type") << to_str(sztype) << endl;
    cealog << to_str(szspec) << endl;
}

void payload::mutate() {
    cealog << endl;

    for(uint64_t loop=0; loop<loop_cnt; loop+=loop_control) {
        for(uint64_t bps=0; bps<burst_per_stream; bps+=bps_control) {
            for(uint64_t ppb=0; ppb<pkts_per_burst; ppb+=ppb_control) {
                // cealog << "Loop:" << loop << " Bps:" << bps << " Ppb:" << ppb << endl;
                //
                switch(ptype) {
                    case Incr_Byte:
                    case Incr_Word:
                    case Decr_Byte:
                    case Decr_Word:
                    case Fixed_Pattern:
                    case Repeat_Pattern: {
                        for(uint64_t nsz=0; nsz<nof_sizes; nsz++) {
                            memcpy(opbuf, buf, size_idx[nsz]);
                        }
                        break;
                        }
                    case Random: {
                        memcpy(opbuf, hdr, hdr_len); // copy hdr
                        for(uint64_t nsz=0; nsz<nof_sizes; nsz++) { // copy random data
                            memcpy(opbuf+hdr_len, buf+(start_idx[nsz]), size_idx[nsz]);
                        }
                        break;
                        }
                }
                nof_pkts++;
            }
            nof_bursts++;
        }
        nof_loops++;
    }
    cealog << endl <<toc(30, "Nof Loops") << nof_loops << endl;
    cealog << toc(30, "Nof Bursts") << nof_bursts << endl;
    cealog << toc(30, "Nof Packets") << nof_pkts << endl;
}
