#include <iostream>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <random>
#include <iomanip>

using namespace std;

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

void print_frame(unsigned char *buf, uint32_t len) {
    ostringstream b("");
    b.setf(ios::hex, ios::basefield);
    b.setf(ios_base::left);
    b << endl;
    b << cea_formatted_hdr("Base Frame");
    
    for (uint32_t idx=0; idx<len; idx++) {
        b << setw(2) << right << setfill('0')<< hex << 
            (uint16_t) buf[idx] << " ";
        if (idx%8==7) b << " ";
        if (idx%16==15) b  << "(" << dec << (idx+1) << ")" << endl;
    }
    b << endl << endl;

    cealog << b.str();
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
