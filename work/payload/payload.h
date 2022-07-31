#include <iostream>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>

using namespace std;

enum cea_field_id {
    PAYLOAD_Type,
    FRAME_Len
};

enum cea_field_generation_type {
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

struct cea_field_generation_spec {
    uint64_t value;
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

void print_frame(unsigned char *buf, uint32_t len, bool single=false) {
    ostringstream b("");
    b.setf(ios::hex, ios::basefield);
    b.setf(ios_base::left);
    b << endl;
    b << cea_formatted_hdr("Base Frame");
    
    for (uint32_t idx=0; idx<len; idx++) {
        b << setw(2) << right << setfill('0')<< hex << 
            (uint16_t) buf[idx] << " ";
        if (!single) {
            if (idx%8==7) b << " ";
            if (idx%16==15) b  << "(" << dec << (idx+1) << ")" << endl;
        }
    }
    b << endl << endl;

    cealog << b.str();
}


void print_uint(uint32_t *buf, uint32_t len, bool single=false) {
    ostringstream b("");
    b.setf(ios::dec, ios::basefield);
    b.setf(ios_base::left);
    b << endl;
    b << cea_formatted_hdr("Integer array");
    
    for (uint32_t idx=0; idx<len; idx++) {
        b << setw(3) << right << (uint16_t) buf[idx] << " ";
        if (!single) {
            if (idx%8==7) b << " ";
            if (idx%16==15) b  << "(" << dec << (idx+1) << ")" << endl;
        }
    }
    b << endl << endl;

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

void randomize_frame_buf(unsigned char *b, uint32_t min, uint32_t max, uint32_t len) {
    random_device rd; // obtain a random number from hardware
    mt19937 gen(rd()); // seed the generator
    uniform_int_distribution<> distr(min, max); // define the range
    for(int idx=0; idx<len; idx++) {
        b[idx] = static_cast<char>(distr(gen));
    }
}

void randomize_uint_buf(uint32_t *b, uint32_t min, uint32_t max, uint32_t len) {
    random_device rd; // obtain a random number from hardware
    mt19937 gen(rd()); // seed the generator
    uniform_int_distribution<> distr(min, max); // define the range
    for(int idx=0; idx<len; idx++) {
        b[idx] = distr(gen);
    }
}

string to_str(cea_field_generation_spec t) {
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

string to_str(cea_field_id t) {
    string name;
    switch(t) {
        case PAYLOAD_Type : { name = "PAYLOAD_Type"; break; }
        case FRAME_Len    : { name = "FRAME_Len   "; break; }
        default           : { name = "undefined   "; break; }
    }
    return cea_trim(name);
}

string to_str(cea_field_generation_type t) {
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

    // fill frame buffer with random values
    // randomize_frame_buf(buf, 0, 255, ONE_MB);


    // prepare a dummy frame
    // fill_frame(buf, 0, hdr_size, 0xff); // dummy MAC header
    // fill_frame(buf, (frm_size-fcs_size), fcs_size, 0x00); // dummy FCS
    // print_frame(buf, frm_size, true);

