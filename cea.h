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
#include <bitset>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define CEA_PACKED __attribute__((packed))

using namespace std;

namespace cea {

// custom output stream to sent messages to both screen and log file    
class outbuf : public streambuf {
protected:
    virtual int overflow(int c) ;
};
outbuf ob;
ostream cealog(&ob);

enum cea_msg_verbosity {
    NONE, // default
    LOW,  // debug only: NONE + pkt details w/o payload bytes
    FULL, // debug only: LOW + pkt payload bytes
};

// TODO: Unused
enum cea_msg_type {
    INFO,
    ERROR,
    WARNING,
    SCBD_ERROR,
    PKT
};

enum cea_pkt_type {
    ETH_V2,
    ETH_LLC,
    ETH_SNAP
};

enum cea_hdr_type {
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
};

enum cea_field_id {
    PKT_Type,
    Network_Hdr,
    Transport_Hdr,
    VLAN_Tags,
    MPLS_Labels,
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
    TCP_Urg,
    TCP_Ack,
    TCP_Psh,
    TCP_Rst,
    TCP_Syn,
    TCP_Fin,
    TCP_Window,
    TCP_Csum,
    TCP_Urg_Ptr,
    TCP_Opts,
    TCP_Pad,
    UDP_Src_Port,
    UDP_Dest_Port,
    UDP_Len,
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
    STREAM_Rate,
    STREAM_Ipg,
    STREAM_Percentage,
    STREAM_Pkts_Per_Sec,
    STREAM_Bit_Rate,
    PAYLOAD_Type,
    Num_Fields
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
    cea_field_generation_type type;
    uint64_t value;
    uint32_t range_start;
    uint32_t range_stop;
    uint32_t range_step;
    uint32_t repeat_after;
    uint32_t step;
};

struct CEA_PACKED cea_field {
    bool touched : 1;
    uint32_t merge : 8;
    uint64_t mask : 64;
    uint32_t id : 32;
    uint32_t len: 32;
    uint32_t offset : 32;
    cea_field_generation_type gen_type : 32;
    uint64_t value: 64;
    uint32_t start: 32;
    uint32_t stop: 32;
    uint32_t step: 32;
    uint32_t repeat: 32;
    char name[32];
    char pad[47];
};

// TODO: Unused
struct CEA_PACKED cea_protocol_sequence {
    cea_hdr_type id : 32;
    uint32_t nof_fields : 32;
    uint32_t seq[32];
};

// forward declaration
class cea_stream;
class cea_proxy;
class cea_manager;

//------------------------------------------------------------------------------
// Manager
//------------------------------------------------------------------------------
class cea_manager {
public:
    cea_manager();
    void add_proxy(cea_proxy *pxy);
    void add_proxy(cea_proxy *pxy, uint32_t cnt);
    void add_stream(cea_stream *stm, cea_proxy *pxy=NULL);
    void add_cmd(cea_stream *stm, cea_proxy *pxy=NULL);
    void exec_cmd(cea_stream *stm, cea_proxy *pxy=NULL);
private:
    vector<cea_proxy*> proxies;
};

//------------------------------------------------------------------------------
// Proxy
//------------------------------------------------------------------------------
class cea_proxy {
public:
    // constructor
    cea_proxy(string name=string("pxy"));

    // add stream to proxy queue
    void add_stream(cea_stream *stm);

    // add a command(cea_stream) to proxy queue
    void add_cmd(cea_stream *stm);

    // execute a command immediately does not add to proxy queue
    void exec_cmd(cea_stream *stm);

    // TODO: move to private before sharing
    void start();

private:

    // set when the proxy object is created
    string proxy_name;

    // automatically assigned when the proxy object is created
    // the value of the field is set from the global variable proxy_id
    uint32_t proxy_id;

    // user's test streams will be pushed into this queue (container1)
    vector<cea_stream*> stmq;

    // handle to the stream being processed
    cea_stream *cur_stm;

    // main thread
    thread worker_tid;
    void worker();
    void start_worker();
    void join_threads();

    // worker modules
    void read_next_stream_from_stmq();
    void set_gen_vars();
    void generate_traffic();

    // buffer to store the generated pkts
    void *pbuf;
    void create_pkt_buffer();
    void release_pkt_buffer();

    friend class cea_manager;
};

// global variable to track proxy and stream id
uint32_t proxy_id = 0;
uint32_t stream_id = 0;

//------------------------------------------------------------------------------
// Stream
//------------------------------------------------------------------------------
class cea_stream {
public:    
    // constructor
    cea_stream(string name=string("stm"));

    // fucntion to set the field to a fixed value
    void set(uint32_t id, uint64_t value);

    // function to assign a field to an inbuilt value generator
    // with default specifications
    void set(uint32_t id, cea_field_generation_type spec);

    // function to assign a field to an inbuilt value generator
    // with custom specifications
    void set(uint32_t id, cea_field_generation_type mspec,
        cea_field_generation_spec vspec);

    // function to add auxillary headers like tags and labels
    // following add(), the relevant fields of the auxillary headers 
    // should also be set failing which default values will be used
    void add(uint32_t id);

    // copy constructor
    cea_stream(const cea_stream &rhs);

    // overload =
    cea_stream& operator = (cea_stream& rhs);

    // overload <<
    friend ostream& operator << (ostream& os, const cea_stream& cmd);

private:
    // set when the proxy object is created
    string stream_name;

    // automatically assigned when the proxy object is created
    // the value of the field is set from the global variable proxy_id
    uint32_t stream_id;

    // (container2) a list of all the field ids (in sequence) of the 
    // selected pkt type and headers output of organize_fields
    vector<cea_field_id> fseq;

    // (container3) a sequence of all the fields ids that needs 
    // generation of values output of trim_static_fields
    vector<uint32_t> cseq;

    // (container4) field matrix
    cea_field fields[cea::Num_Fields];

    // check if a field has been modified by user
    bool is_touched(cea_field_id fid);

    // check if a field is not aligned to 8-bit boundary
    uint32_t is_merge(cea_field_id fid);

    // get the fixed or current value of the field
    uint32_t value_of(cea_field_id fid);

    void organize_fields();
    void trim_static_fields();
    void prune_stream();
    void build_baseline_pkt();

    // baseline pkt
    unsigned char *base_pkt;
    uint32_t baseline_pkt_len;
    void print_baseline_pkt();

    // reset to default values
    void reset();

    // overloads
    void do_copy (const cea_stream *rhs);
    string describe() const;

    friend class cea_proxy;
};

} // namespace
