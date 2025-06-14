#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

using namespace std;

namespace cea {

class outbuf : public streambuf {
protected:
    virtual int overflow(int c);
} ob;

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
    META,
    PROPERTIES
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
    STREAM_Burst_Size,
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

enum cea_stream_feature_id {
    PCAP_Record_Tx_Enable,
    PCAP_Record_Rx_Enable
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
    Value_List,
    Increment,
    Decrement,
    Random,
    Increment_Byte,
    Decrement_Byte,
    Increment_Word,
    Decrement_Word,
    Continuous,
    Bursty,
    Stop_After_Stream,
    Goto_Next_Stream
};

struct cea_field_genspec {
    cea_gen_type gen_type;
    struct {
        uint64_t value;
        uint64_t step;
        uint64_t min;
        uint64_t max;
        uint64_t count;
        uint64_t repeat;
        uint64_t mask;
        uint64_t seed;
        uint64_t start;
        bool error;
        vector<uint64_t> values;
    } nmr;
    struct {
        string   value;
        uint64_t step;
        string   min;
        string   max;
        uint64_t count;
        uint64_t repeat;
        string   mask;
        uint64_t seed;
        string   start;
        bool error;
        vector<string> values;
    } str;
};

// forward declaration
class cea_stream;
class cea_port;
class cea_testbench;
class cea_header;
class cea_field;
class cea_udf;

//------------------------------------------------------------------------------
// Top level software class (sw testbench)
//------------------------------------------------------------------------------

class cea_testbench {
public:
    cea_testbench(); // TODO add socket port number for external interfacing
    ~cea_testbench();
    void add_port(cea_port *port);
    void add_stream(cea_stream *stream, cea_port *port = NULL);
    void add_cmd(cea_stream *stream, cea_port *port = NULL);
    void exec_cmd(cea_stream *stream, cea_port *port = NULL);
    void start(cea_port *port = NULL);
    void stop(cea_port *port = NULL);
    void pause(cea_port *port = NULL);
private:
    class core;
    unique_ptr<core> impl;
};

//------------------------------------------------------------------------------
// Port Class
//------------------------------------------------------------------------------

class cea_port {
public:
    cea_port(string name = "port");
    ~cea_port();
    void add_stream(cea_stream *stream);
    void add_cmd(cea_stream *stream);
    void exec_cmd(cea_stream *stream);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_testbench;
};

//------------------------------------------------------------------------------
// Stream Class
//------------------------------------------------------------------------------

class cea_stream {
public:    
    cea_stream(string name = "stream");
    ~cea_stream();
    void set(cea_field_id id, uint64_t value); // to set property only
    void set(cea_field_id id, cea_field_genspec spec); // to set property only
    void set(cea_stream_feature_id feature, bool mode);
    void add_header(cea_header *header);
    void add_udf(cea_field *field);
    // TODO Support AVIP type test case
    // TODO Support AVIP type custom payload
    //
    // TODO Support API to quickly set payload without a spec
    // void set(cea_field_id id, enum of PREDFINED_PAYLOAD_PATTERN);
    // void set(cea_field_id id, string pattern, bool fill=true);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_port;
};

//------------------------------------------------------------------------------
// Header Class
//------------------------------------------------------------------------------

class cea_header {
public:
    cea_header(cea_header_type hdr_type);
    ~cea_header();
    void set(cea_field_id id, uint64_t value);
    void set(cea_field_id id, string value);
    void set(cea_field_id id, cea_field_genspec spec);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_stream;
};

//------------------------------------------------------------------------------
// Field Class
//------------------------------------------------------------------------------

class cea_field {
public:
    cea_field(cea_field_id id);
    ~cea_field();
    void set(uint64_t value);
    void set(string value);
    void set(cea_field_genspec spec);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_stream;
};

//------------------------------------------------------------------------------
// User defined field Class
//------------------------------------------------------------------------------

class cea_udf {
public:
    cea_udf();
    ~cea_udf();
    void set(cea_field_genspec spec);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_stream;
};

} // namespace 
