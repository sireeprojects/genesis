#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

using namespace std;

namespace cea {

class outbuf : public streambuf {
protected:
    virtual int overflow(int c) ;
};
outbuf ob;

// A custom output stream. Usage is similar to cout except that cout writes
// messages only to the screen, but cealog writes the message to the screen
// as well as the log file. Usage example:
//     cealog << "Frame recevied: " << frm_count << endl;
ostream cealog(&ob);

enum cea_header_type {
    MAC,
    LLC,
    SNAP,
    VLAN,
    MPLS,
    IPv4,
    IPv6,
    ARP,
    TCP,
    UDP,
    PAUSE,
    PFC,
    UDP_PHDR,
    TCP_PHDR,
    META
};

enum cea_field_id {
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
    MPLS_Label,
    MPLS_Exp,
    MPLS_Stack,
    MPLS_Ttl,
    VLAN_Tpi,
    VLAN_Tci_Pcp,
    VLAN_Tci_Cfi,
    VLAN_Vid,
    MAC_Control,
    MAC_Control_Opcode,
    Pause_Quanta,
    Priority_En_Vector,
    Pause_Quanta_0,
    Pause_Quanta_1,
    Pause_Quanta_2,
    Pause_Quanta_3,
    Pause_Quanta_4,
    Pause_Quanta_5,
    Pause_Quanta_6,
    Pause_Quanta_7,
    FRAME_Len,
    PAYLOAD_Pattern,
    STREAM_Traffic_Type,
    STREAM_Traffic_Control,
    STREAM_Ipg,
    STREAM_Isg,
    STREAM_Ibg,
    STREAM_Bandwidth,
    STREAM_Start_Delay,
    UDF,
    META_Len,
    META_Ipg,
    META_Preamble,
    META_Pad1,
    META_Pad2,
    META_Pad3,
    META_Pad4,
    META_Pad5,
    META_Pad6,
    Zeros_8Bit,
    TCP_Total_Len,
    Num_Fields
};

enum cea_field_type {
    Integer,
    Pattern
};

enum cea_unit {
    Percent,
    Frames_Per_Sec,
    Millisecond,
    Nanosecond,
    Bytes,
    Bits_Per_Sec,
    Kilobits_Per_Sec,
    Megabits_Per_Sec
};

enum cea_gen_type {
    Fixed_Value,
    Fixed_Pattern,
    Value_List,
    Pattern_List,
    Increment,
    Decrement,
    Random,
    Increment_Bytes,
    Decrement_Byte,
    Increment_Word,
    Decrement_Word,
    Continuous,
    Bursty,
    Stop_After_Stream,
    Goto_Next_Stream
};

struct cea_gen_spec {
    cea_gen_type gen_type;
    uint64_t value;
    string pattern;
    uint32_t step;
    uint32_t min;
    uint32_t max;
    uint32_t count;
    uint32_t repeat;
    uint32_t mask;
    uint32_t seed;
    uint32_t start;
    bool make_error;
    vector<uint64_t> value_list;
    vector<string> pattern_list;
};

struct cea_field {
    bool is_mutable;
    uint32_t merge;
    uint32_t id;
    uint32_t len;
    uint32_t offset;
    bool proto_list_specified;
    bool auto_field;
    string name;
    cea_field_type type;
    cea_gen_spec spec;
};

// forward declaration
class cea_stream;
class cea_proxy;
class cea_manager;
class cea_header;

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
    class core;
    unique_ptr<core> impl;
};

//------------------------------------------------------------------------------
// Proxy
//------------------------------------------------------------------------------
class cea_proxy {
public:
    cea_proxy(string name = "pxy");
    ~cea_proxy();
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_manager;
};

//------------------------------------------------------------------------------
// Stream
//------------------------------------------------------------------------------
class cea_stream {
public:    
    cea_stream(string name = "stm");
    ~cea_stream();
    void set(cea_field_id id, uint64_t value);
    void set(cea_field_id id, cea_gen_spec spec);
    cea_header *create_header(cea_header_type type);
    void add_header(cea_header *hdr);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_proxy;
};

//------------------------------------------------------------------------------
// Header Class
//------------------------------------------------------------------------------
class cea_header {
public:
    cea_header(cea_header_type hdr);
    ~cea_header() = default;
    void set(cea_field_id id, uint64_t value);
    void set(cea_field_id id, cea_gen_spec spec);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_stream;
};

} // namespace 
