#include <iostream>
#include <vector>
#include <thread>
#include <cstdint>
#include <sys/mman.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
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
    NONE,
    LOW,
    FULL,
} msg_verbosity;

typedef enum {
    INFO,
    ERROR,
    WARNING,
    SCBD_ERROR,
    PKT
} msg_type;

typedef enum {
    CEA_SHORT,
    CEA_LONG
} desc_type;

// typedef enum {
//     UNDEF,
//     TEST,
//     CFG,
//     CMD,
//     ERR,
//     WARN,
//     EVENT,
//     TXN
// } msg_type;

typedef enum {
    CEA_V2,
    CEA_RAW
} frame_type;

typedef enum {
    CEA_MAC_Preamble,
    CEA_MAC_Dest_Addr,
    CEA_MAC_Src_Addr,
    CEA_MAC_Len,
    CEA_MAC_Ether_Type,
    CEA_MAC_Fcs,
    CEA_IPv4_Version,
    CEA_IPv4_IHL,
    CEA_IPv4_Tos,
    CEA_IPv4_Total_Len,
    CEA_IPv4_Id,
    CEA_IPv4_Flags,
    CEA_IPv4_Frag_Offset,
    CEA_IPv4_TTL,
    CEA_IPv4_Protocol,
    CEA_IPv4_Hdr_Csum,
    CEA_IPv4_Src_Addr,
    CEA_IPv4_Dest_Addr,
    CEA_IPv4_Opts,
    CEA_IPv4_Pad,
    CEA_UDP_Src_Port,
    CEA_UDP_Dest_Port,
    CEA_UDP_len,
    CEA_UDP_Csum,
    CEA_STREAM_Type,
    CEA_STREAM_Pkts_Per_Burst,
    CEA_STREAM_Burst_Per_Stream,
    CEA_STREAM_Inter_Burst_Gap,
    CEA_STREAM_Inter_Stream_Gap,
    CEA_STREAM_Start_Delay,
    CEA_STREAM_Rate_Type,
    CEA_STREAM_rate,
    CEA_STREAM_Ipg,
    CEA_STREAM_Percentage,
    CEA_STREAM_PktsPerSec,
    CEA_STREAM_BitRate,
    CEA_PAYLOAD_Type,
    CEA_NumFields
} field_id;

typedef enum {
    CEA_Fixed,            
    CEA_Random,           
    CEA_Random_in_Range,  
    CEA_Increment,        
    CEA_Decrement,        
    CEA_Increment_Cycle,  
    CEA_Decrement_Cycle, 
    CEA_Incr_Byte,        
    CEA_Incr_Word,          
    CEA_Decr_Byte,           
    CEA_Decr_Word,            
    CEA_Repeat_Pattern,        
    CEA_Fixed_Pattern,          
    CEA_Continuous_Pkts,
    CEA_Continuous_Burst,
    CEA_Stop_After_Stream,
    CEA_Goto_Next_Stream,
    CEA_Ipg,
    CEA_Percentage,
    CEA_Pkts_Per_Sec,
    CEA_Bit_Rate
} field_modifier;

struct stream_control_specifications {
    field_modifier stream_type;
    uint32_t pkts_per_burst;
    uint32_t burst_per_stream;
    uint32_t inter_burst_gap;
    uint32_t inter_stream_gap;
    uint32_t start_delay;
    field_modifier rate_type;
    uint32_t rate;
};

struct value_spec {
    field_modifier type;
    uint64_t value;
    uint32_t range_start;
    uint32_t range_stop;
    uint32_t range_step;
    uint32_t repeat_after;
    uint32_t step;
};

struct data_specifications {
    field_modifier size;
    uint32_t inc_min;
    uint32_t inc_max;
    uint32_t inc_step;
    uint32_t dec_min;
    uint32_t dec_max;
    uint32_t dec_step;
    uint32_t repeat_after;
    field_modifier data;
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
    field_modifier modifier : 32;
    uint64_t value: 64;
    uint32_t start: 32;
    uint32_t stop: 32;
    uint32_t step: 32;
    uint32_t repeat: 32;
    char name[32];
    char pad[47];
};

typedef enum {
    HEX,
    DEC,
    STR
} display_type;

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
// notes:
// stream should only be copied into queues
class cea_stream {
public:    
    cea_stream();
    char* pack();
    void unpack(char *data);
    void do_copy (const cea_stream* rhs);
    friend ostream& operator << (ostream& os, const cea_stream& cmd);
    string describe() const;
    cea_stream (const cea_stream& rhs);
    cea_stream& operator = (cea_stream& rhs);

    cea_field fields[64];
    void set(uint32_t id, uint64_t value);
    void set(uint32_t id, field_modifier spec);
};

class cea_field_future {
public:
    uint32_t id;
    // used in printing
    string name;
    display_type display;
    string describe(desc_type t=CEA_SHORT) const;
    // standard length of the protocol field
    uint32_t len;
    // staandard offset of the protocol field
    uint32_t offset;
    value_spec spec;
    cea_field_future();
    friend ostream& operator << (ostream& os, const cea_field_future& cmd);
    void set_defaults();
    bool touched;
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
