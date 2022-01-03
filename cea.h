/*
TODO:
Implementation:
--combine fields that are not a multiple of a byte
    * might complicate set api, merge fields under consolidate fn
--reimplement reset to carry valid default values

Test:
*/

#include <iostream>
#include <vector>
#include <thread>
#include <cstdint>
#include <sys/mman.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <map>
#define CEA_PACKED __attribute__((packed))

using namespace std;

namespace cea {

class outbuf : public streambuf {
protected:
    virtual int overflow(int c) ;
};
outbuf ob;
ostream cealog(&ob);

typedef enum {
    NONE, // default
    LOW,  // NONE + pkt details w/o payload bytes
    FULL, // LOW + pkt payload bytes
} cea_msg_verbosity;

typedef enum {
    INFO,
    ERROR,
    WARNING,
    SCBD_ERROR,
    PKT
} cea_msg_type;

typedef enum {
    SHORT,
    LONG
} cea_pkt_print_type;

typedef enum {
    V2,
    RAW
} cea_pkt_type;

typedef enum {
    MAC,
    VLAN,
    MPLS,
    LLC,
    SNAP,
    IPv4,
    IPv6,
    ARP,
    TCP,
    UDP
} cea_pkt_hdr_type;

typedef enum {
    MAC_Preamble,
    MAC_Dest_Addr,
    MAC_Src_Addr,
    MAC_Len,
    MAC_Ether_Type,
    MAC_Fcs,
    IPv4_Version,
    IPv4_IHL,
    IPv4_Tos,
    IPv4_Total_Len,
    IPv4_Id,
    IPv4_Flags,
    IPv4_Frag_Offset,
    IPv4_TTL,
    IPv4_Protocol,
    IPv4_Hdr_Csum,
    IPv4_Src_Addr,
    IPv4_Dest_Addr,
    IPv4_Opts,
    IPv4_Pad,
    UDP_Src_Port,
    UDP_Dest_Port,
    UDP_len,
    UDP_Csum,
    STREAM_Type,
    STREAM_Pkts_Per_Burst,
    STREAM_Burst_Per_Stream,
    STREAM_Inter_Burst_Gap,
    STREAM_Inter_Stream_Gap,
    STREAM_Start_Delay,
    STREAM_Rate_Type,
    STREAM_rate,
    STREAM_Ipg,
    STREAM_Percentage,
    STREAM_PktsPerSec,
    STREAM_BitRate,
    PAYLOAD_Type,
    NumFields
} cea_field_id;

typedef enum {
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
} cea_field_modifier;

struct stream_control_specifications {
    cea_field_modifier stream_type;
    uint32_t pkts_per_burst;
    uint32_t burst_per_stream;
    uint32_t inter_burst_gap;
    uint32_t inter_stream_gap;
    uint32_t start_delay;
    cea_field_modifier rate_type;
    uint32_t rate;
};

struct cea_value_spec {
    cea_field_modifier type;
    uint64_t value;
    uint32_t range_start;
    uint32_t range_stop;
    uint32_t range_step;
    uint32_t repeat_after;
    uint32_t step;
};

struct cea_data_specifications {
    cea_field_modifier size;
    uint32_t inc_min;
    uint32_t inc_max;
    uint32_t inc_step;
    uint32_t dec_min;
    uint32_t dec_max;
    uint32_t dec_step;
    uint32_t repeat_after;
    cea_field_modifier data;
    uint32_t increment_word_size;
    uint32_t decrement_word_size;
    string pattern;
    uint32_t repeat_count;
};

struct CEA_PACKED cea_field {
    bool touched : 1;
    bool merge : 1;
    uint64_t mask : 64;
    uint32_t id : 32;
    uint32_t len: 32;
    uint32_t ofset : 32;
    cea_field_modifier modifier : 32;
    uint64_t value: 64;
    uint32_t start: 32;
    uint32_t stop: 32;
    uint32_t step: 32;
    uint32_t repeat: 32;
    char name[32];
    char pad[47];
};

struct CEA_PACKED cea_protocol_sequence {
    cea_pkt_hdr_type id : 32;
    uint32_t len : 32;
    uint32_t seq[32];
};

typedef enum {
    HEX,
    DEC,
    STR
} cea_field_print_type;

// forward declaration
class cea_stream;
class cea_proxy;
class cea_manager;

//---------
// Manager
//---------
class cea_manager {
public:
    cea_manager();
    void add_proxy(cea_proxy *pxy);
    void add_proxy(cea_proxy *pxy, uint32_t cnt);
    void add_stream(cea_stream *stm, cea_proxy *pxy=NULL);
    void add_cmd(cea_stream *stm, cea_proxy *pxy=NULL);
    void exec_cmd(cea_stream *stm, cea_proxy *pxy=NULL);
    void testfn(cea_proxy *s);
private:
    vector<cea_proxy*> proxies;
};

//-------
// Proxy
//-------
class cea_proxy {
public:
    cea_proxy();
    void add_stream(cea_stream *stm);
    void add_cmd(cea_stream *stm);
    void exec_cmd(cea_stream *stm);
    void testfn(cea_stream *s);
    uint32_t pid;
    void start();

private:
    uint32_t port_num;
    vector<cea_stream*> streamq;
    thread w;

    void worker(); // main thread
    void start_worker(); // on stream start
    void read();
    void consolidate();
    void set_gen_vars();
    void generate();
    void *fbuf;
    void create_frm_buffer();
    void release_frm_buffer();
};

uint32_t pid = 0;

//--------
// Stream
//--------
class cea_stream {
public:    
    cea_stream();
    cea_stream (const cea_stream& rhs);
    cea_stream& operator = (cea_stream& rhs);
    friend ostream& operator << (ostream& os, const cea_stream& cmd);
    void set(uint32_t id, uint64_t value);
    void set(uint32_t id, cea_field_modifier spec);
    void testfn();
    string name;
private:
    char* pack();
    void unpack(char *data);
    void do_copy (const cea_stream* rhs);
    string describe() const;
    cea_field fields[cea::NumFields];
    void reset();
};

template<typename ... Args>
string string_format(const string& format, Args ... args) {
    size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1;
    if(size <= 0){ throw runtime_error("Error during formatting."); }
    unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return string(buf.get(), buf.get() + size - 1);
}

#define CEA_MSG(...) \
    cealog << string_format(__VA_ARGS__) << endl; 

#ifdef CEA_DEBUG
#define CEA_DBG(...) { CEA_MSG(__VA_ARGS__) }
#else
#define CEA_DBG(...) {}
#endif

} // namespace
