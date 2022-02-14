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
#include <mutex>
#include <bitset>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>

#define CEA_PACKED __attribute__((packed))

using namespace std;

namespace cea {

// custom output stream
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

enum cea_pkt_type {
    ETH_V2,
    ETH_LLC,
    ETH_SNAP
};

enum cea_hdr_type {
    MAC,
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
    VLAN_Tag,
    MPLS_Hdr,
    MAC_Preamble,
    MAC_Dest_Addr,
    MAC_Src_Addr,
    MAC_Len,
    MAC_Ether_Type,
    MAC_Fcs,
    LLC_Dsap,
    LLC_Ssap,
    LLC_Control,
    SNAP_Oui,
    SNAP_Pid,
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
    PAYLOAD_Type,
    PAYLOAD_Len,
    UDF1,
    UDF2,
    UDF3,
    UDF4,
    UDF5,
    UDF6,
    UDF7,
    UDF8,
    MPLS_01_Label,
    MPLS_01_Cos,
    MPLS_01_Stack,
    MPLS_01_Ttl,
    MPLS_02_Label,
    MPLS_02_Cos,
    MPLS_02_Stack,
    MPLS_02_Ttl,
    MPLS_03_Label,
    MPLS_03_Cos,
    MPLS_03_Stack,
    MPLS_03_Ttl,
    MPLS_04_Label,
    MPLS_04_Cos,
    MPLS_04_Stack,
    MPLS_04_Ttl,
    MPLS_05_Label,
    MPLS_05_Cos,
    MPLS_05_Stack,
    MPLS_05_Ttl,
    MPLS_06_Label,
    MPLS_06_Cos,
    MPLS_06_Stack,
    MPLS_06_Ttl,
    MPLS_07_Label,
    MPLS_07_Cos,
    MPLS_07_Stack,
    MPLS_07_Ttl,
    MPLS_08_Label,
    MPLS_08_Cos,
    MPLS_08_Stack,
    MPLS_08_Ttl,
    VLAN_01_Tpi,
    VLAN_01_Tci_Pcp,
    VLAN_01_Tci_Cfi,
    VLAN_01_Vid,
    VLAN_02_Tpi,
    VLAN_02_Tci_Pcp,
    VLAN_02_Tci_Cfi,
    VLAN_02_Vid,
    VLAN_03_Tpi,
    VLAN_03_Tci_Pcp,
    VLAN_03_Tci_Cfi,
    VLAN_03_Vid,
    VLAN_04_Tpi,
    VLAN_04_Tci_Pcp,
    VLAN_04_Tci_Cfi,
    VLAN_04_Vid,
    VLAN_05_Tpi,
    VLAN_05_Tci_Pcp,
    VLAN_05_Tci_Cfi,
    VLAN_05_Vid,
    VLAN_06_Tpi,
    VLAN_06_Tci_Pcp,
    VLAN_06_Tci_Cfi,
    VLAN_06_Vid,
    VLAN_07_Tpi,
    VLAN_07_Tci_Pcp,
    VLAN_07_Tci_Cfi,
    VLAN_07_Vid,
    VLAN_08_Tpi,
    VLAN_08_Tci_Pcp,
    VLAN_08_Tci_Cfi,
    VLAN_08_Vid,
    Num_VLAN_Tags,
    Num_MPLS_Hdrs,
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
    uint64_t value;
    uint32_t range_start;
    uint32_t range_stop;
    uint32_t range_step;
    uint32_t repeat_after;
    uint32_t step;
};

struct cea_field {
    bool touched;
    uint32_t merge;
    bool added;
    uint32_t stack;
    uint32_t id;
    uint32_t len;
    uint32_t offset;
    cea_field_generation_type gen_type;
    uint64_t value;
    uint32_t start;
    uint32_t stop;
    uint32_t step;
    uint32_t repeat;
    string name;
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
    void extract_traffic_parameters();
    void begin_mutation();

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
    void set(cea_field_id id, uint64_t value);

    // function to assign a field to an inbuilt value generator
    // with default specifications
    void set(cea_field_id id, cea_field_generation_type spec);

    // function to assign a field to an inbuilt value generator
    // with custom specifications
    void set(cea_field_id id, cea_field_generation_type mspec,
        cea_field_generation_spec vspec);

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
    // selected pkt type and headers output of arrange_fields_in_sequence
    // vector<cea_field_id> fseq;
    vector<uint32_t> fseq;

    // (container3) a sequence of all the fields ids that needs 
    // generation of values output of purge_static_fields
    vector<uint32_t> cseq;

    // (container4) field matrix
    // cea_field fields[cea::Num_Fields];
    vector<cea_field> fields;

    // check if a field has been modified by user
    bool is_touched(uint32_t fid);

    // check if a field is not aligned to 8-bit boundary
    uint32_t is_merge(cea_field_id fid);

    // get the fixed or current value of the field
    uint32_t value_of(cea_field_id fid);

    void arrange_fields_in_sequence();
    void purge_static_fields();
    void prune();
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

    // runtime and contexts
    uint32_t ipv4_offset;
    uint32_t tcp_offset;

    friend class cea_proxy;
};

} // namespace 
