#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

#define CEA_PACKED __attribute__((packed))

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

enum cea_msg_verbosity {
    NONE, // default
    LOW,  // debug only: NONE + frame details w/o payload bytes
    FULL, // debug only: LOW + frame payload bytes
};

enum cea_frame_type {
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
    UDP,
    PAUSE,
    PFC,
    UDP_PHDR,
    TCP_PHDR
};

enum cea_field_id {
    FRAME_Type,
    FRAME_Len,
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
    PAYLOAD_Pattern,
    UDF1,
    UDF2,
    UDF3,
    UDF4,
    UDF5,
    UDF6,
    UDF7,
    UDF8,
    MPLS_01_Label,
    MPLS_01_Exp,
    MPLS_01_Stack,
    MPLS_01_Ttl,
    MPLS_02_Label,
    MPLS_02_Exp,
    MPLS_02_Stack,
    MPLS_02_Ttl,
    MPLS_03_Label,
    MPLS_03_Exp,
    MPLS_03_Stack,
    MPLS_03_Ttl,
    MPLS_04_Label,
    MPLS_04_Exp,
    MPLS_04_Stack,
    MPLS_04_Ttl,
    MPLS_05_Label,
    MPLS_05_Exp,
    MPLS_05_Stack,
    MPLS_05_Ttl,
    MPLS_06_Label,
    MPLS_06_Exp,
    MPLS_06_Stack,
    MPLS_06_Ttl,
    MPLS_07_Label,
    MPLS_07_Exp,
    MPLS_07_Stack,
    MPLS_07_Ttl,
    MPLS_08_Label,
    MPLS_08_Exp,
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
    STREAM_Crc_Enable,
    STREAM_Timestamp_Enable,
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
    Zeros_8Bit,
    TCP_Total_Len,
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
    string pattern;
    uint32_t start;
    uint32_t stop;
    uint32_t step;
    uint32_t repeat;
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
    class core;
    unique_ptr<core> impl;
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

    // add a command (in the form of cea_stream) to proxy queue
    void add_cmd(cea_stream *stm);

    // execute a command immediately does not add to proxy queue
    void exec_cmd(cea_stream *stm);

    ~cea_proxy();

    // TODO: move to private before sharing
    void start();

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
    // constructor
    cea_stream(string name=string("stm"));

    // fucntion to set the field to a fixed value
    void set(cea_field_id id, uint64_t value);

    // function to assign a field to an inbuilt value generator
    // with custom specifications
    void set(cea_field_id id, cea_field_generation_spec spec);

    // copy constructor
    cea_stream(const cea_stream &rhs);

    // overload =
    cea_stream& operator = (cea_stream& rhs);

    // overload <<
    friend ostream& operator << (ostream& os, const cea_stream &cmd);

    ~cea_stream();

private:
    class core;
    unique_ptr<core> impl;
    friend class cea_proxy;
};

} // namespace 
