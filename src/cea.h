#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <fstream>
#include <locale>
#include <cstdio>
#include <streambuf>
#define PACKED __attribute__((packed))
using namespace std;

namespace cea {

class outbuf : public streambuf {
protected:
    virtual int overflow(int c) ;
};
outbuf ob;
ostream cealogger(&ob);

typedef enum {
    low,
    full,
} msg_verbosity_level;

typedef enum {
    V2,
    raw
} frame_type;

typedef enum {
    mac_preamble,
    mac_dest_addr,
    mac_src_addr,
    mac_len,
    mac_ether_type,
    mac_fcs,
    ipv4_version,
    ipv4_ihl,
    ipv4_tos,
    ipv4_total_len,
    ipv4_id,
    ipv4_flags,
    ipv4_frag_offset,
    ipv4_ttl,
    ipv4_protocol,
    ipv4_hdr_csum,
    ipv4_src_addr,
    ipv4_dest_addr,
    ipv4_opts,
    ipv4_pad,
    udp_src_port,
    udp_dest_port,
    udp_len,
    udp_csum,
    stream_type,
    stream_pkts_per_burst,
    stream_burst_per_stream,
    stream_inter_burst_gap,
    stream_inter_stream_gap,
    stream_start_delay,
    stream_rate_type,
    stream_ipg,
    stream_percentage,
    stream_pkts_per_aec,
    stream_bit_rate,
    payload_type,
    fields_size
} field_id;

typedef enum {
    // value modifiers
    fixed,                  // init value
    random,                 // from 0 to max int
    rrandom,                // random in range
    increment,              // from start to stop with steps
    decrement,              // from start to stop with steps
    cincrement,             // starts from 0 and loops at repeat after
    cdecrement,             // starts from ff and loops at repeat after
    // data modifiers
    incr_byte,              // start from zero
    incr_word,              // start from zero
    decr_byte,              // start from FF
    decr_word,              // start from FF
    repeat_pattern,        // repeat a data pattern
    fixed_pattern,          // use data pattern only once, rest 0s
    // stream modifiers
    continuous_pkts,            
    continuous_burst,          
    stop_after_stream,    
    goto_next_stream,
    // stream rate modifiers
    ipg,
    percentage,
    pkts_per_sec,
    bit_rate
} field_modifier;

struct PACKED ceaField {
    bool touched : 1;
    bool merge : 1;
    uint64_t mask : 64;
    uint32_t id : 32;
    uint32_t len: 32;
    uint32_t ofset : 32;
    field_modifier modifier : 32;
    uint64_t value: 64;
    uint32_t start: 32;
    uint32_t stop: 32;
    uint32_t step: 32;
    uint32_t repeat: 32;
    char name[32];
    char pad[47];
};

class cea_stream {
public:
    void set(uint32_t id, uint64_t value);
    void set(uint32_t id, field_modifier spec);
    string describe() const;
    cea_stream();
    friend ostream& operator << (ostream& os, const cea_stream& cmd);
private:
    ceaField fld[64];
    void consolidate();
    string name;
};

} // namespace

//---{ Definitions }--------------------------------------------------------

namespace cea {

ofstream logfile;
msg_verbosity_level global_verbosity;

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

class init_lib {
public:
    init_lib() {
        logfile.open("run.log", ofstream::out);
        if (!logfile.is_open()) {
            cout << "Error creating logfile. Aborting..." << endl;
            exit(1);
        }
    }
    ~init_lib() { logfile.close(); }
};

init_lib init_lib;

} // namespace
