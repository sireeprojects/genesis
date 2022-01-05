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
#include <unistd.h>
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
    Ethernet_V2,
    Ethernet_LLC,
    Ethernet_SNAP
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
    PKT_Type,
    PKT_Network_Hdr,
    PKT_Transport_Hdr,
    PKT_VLAN_Tags,
    PKT_MPLS_Labels,
    MAC_Preamble,
    MAC_Dest_Addr,
    MAC_Src_Addr,
    MAC_Len,
    MAC_Ether_Type,
    MAC_Fcs,
    VLAN_Tpi,
    VLAN_Tci_Pcp,
    VLAN_Tci_Cfi,
    VLAN_Vid,
    LLC_Dsap,
    LLC_Ssap,
    LLC_Control,
    SNAP_Oui,
    SNAP_Pid,
    MPLS_Label,
    MPLS_Cos,
    MPLS_Stack,
    MPLS_Ttl,
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
    IPv6_Version,
    IPv6_Traffic_Class,
    IPv6_Flow_Label,
    IPv6_Payload_Len,
    IPv6_Next_Hdr,
    IPv6_Hop_Limit,
    IPv6_Src_Addr,
    IPv6_Dest_Addr,
    TCP_Src_Port,
    TCP_Dest_Port,
    TCP_Seq_Num,
    TCP_Ack_Num,
    TCP_Data_Offset,
    TCP_Reserved,
    TCP_URG,
    TCP_ACK,
    TCP_PSH,
    TCP_RST,
    TCP_SYN,
    TCP_FIN,
    TCP_Window,
    TCP_Csum,
    TCP_UrgPtr,
    TCP_Opts,
    TCP_Pad,
    UDP_Src_Port,
    UDP_Dest_Port,
    UDP_len,
    UDP_Csum,
    ARP_Hw_Type,
    ARP_Proto_Type,
    ARP_Hw_Len,
    ARP_Proto_Len,
    ARP_Opcode,
    ARP_Sender_Hw_Addr,
    ARP_Sender_Proto_addr,
    ARP_Target_Hw_Addr,
    ARP_Target_Proto_Addr,
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

struct cea_value_spec {
    cea_field_modifier type;
    uint64_t value;
    uint32_t range_start;
    uint32_t range_stop;
    uint32_t range_step;
    uint32_t repeat_after;
    uint32_t step;
};

struct CEA_PACKED cea_field {
    bool touched : 1;
    bool merge : 1;
    uint64_t mask : 64;
    uint32_t id : 32;
    uint32_t len: 32;
    uint32_t offset : 32;
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
    cea_proxy(string name);
    void add_stream(cea_stream *stm);
    void add_cmd(cea_stream *stm);
    void exec_cmd(cea_stream *stm);
    void testfn();
    uint32_t pid;
    void start();
    string name();

private:
    uint32_t port_num;
    vector<cea_stream*> streamq;
    cea_stream *cur_stream;
    vector<uint32_t> fseq; // output of generate_field_sequence
    vector<uint32_t> consolidated_fseq; // output of consolidate_fields
    thread w;
    string pname;

    void worker(); // main thread
    void start_worker(); // on stream start
    void read();
    void generate_field_sequence();
    void consolidate_fields();
    void set_gen_vars();
    void generate();
    void join_threads();
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
    void set(uint32_t id, cea_field_modifier mspec, cea_value_spec vspec);
    void add(uint32_t id); // adding tags

    void testfn();
    bool is_touched(cea_field_id fid);
    uint32_t value_of(cea_field_id fid);
    cea_field fields[cea::NumFields];
private:
    char* pack();
    void unpack(char *data);
    void do_copy (const cea_stream* rhs);
    string describe() const;
    void reset();
    string name;
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
