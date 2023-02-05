#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <memory>

using namespace std;

namespace cea {

enum cea_hdr_type {
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

enum cea_uint {
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
    uint64_t value;
    string pattern;
    vector<uint64_t> value_list;
    vector<string> pattern_list;
    uint32_t step;
    uint32_t min;
    uint32_t max;
    uint32_t count;
    uint32_t repeat;
    uint32_t mask;
    uint32_t seed;
    uint32_t start;
    bool make_error;
};

struct cea_field {
    bool touched;
    uint32_t merge;
    uint32_t id;
    uint32_t len;
    uint32_t offset;
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
    bool proto_list_specified;
    bool auto_field;
    bool make_error;
    string name;
    cea_field_type type;
    vector<uint64_t> value_list;
    vector<string> string_list;
};

map <cea_hdr_type, vector <cea_field_id>> htof = {
    {META,  {
            META_Len,
            META_Ipg,
            META_Preamble,
            META_Pad1,
            META_Pad2,
            META_Pad3,
            META_Pad4,
            META_Pad5,
            META_Pad6
            }},
    {MAC,   {
            MAC_Dest_Addr,
            MAC_Src_Addr
            }},
    {LLC,   {
            LLC_Dsap,
            LLC_Ssap,
            LLC_Control
            }},
    {SNAP,  {
            SNAP_Oui,
            SNAP_Pid
            }},
    {VLAN,  {
            VLAN_Tpi,
            VLAN_Tci_Pcp,
            VLAN_Tci_Cfi,
            VLAN_Vid
            }},
    {MPLS,  {
            MPLS_Label,
            MPLS_Exp,
            MPLS_Stack,
            MPLS_Ttl
            }},
    {IPv4,  {
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
            IPv4_Pad
            }},
    {IPv6,  {
            IPv6_Version,
            IPv6_Traffic_Class,
            IPv6_Flow_Label,
            IPv6_Payload_Len,
            IPv6_Next_Hdr,
            IPv6_Hop_Limit,
            IPv6_Src_Addr,
            IPv6_Dest_Addr
            }},
    {ARP,   {
            ARP_Hw_Type,
            ARP_Proto_Type,
            ARP_Hw_Len,
            ARP_Proto_Len,
            ARP_Opcode,
            ARP_Sender_Hw_Addr,
            ARP_Sender_Proto_addr,
            ARP_Target_Hw_Addr,
            ARP_Target_Proto_Addr
            }},
    {TCP,   {
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
            TCP_Pad
            }},
    {UDP,   {
            UDP_Src_Port,
            UDP_Dest_Port,
            UDP_Len,
            UDP_Csum
            }},
    {PAUSE, {
            MAC_Control_Opcode,
            Pause_Quanta
            }},
    {PFC,   {
            MAC_Control_Opcode,
            Priority_En_Vector,
            Pause_Quanta_0,
            Pause_Quanta_1,
            Pause_Quanta_2,
            Pause_Quanta_3,
            Pause_Quanta_4,
            Pause_Quanta_5,
            Pause_Quanta_6,
            Pause_Quanta_7
            }},
    {UDP_PHDR, {
            IPv4_Src_Addr,
            IPv4_Dest_Addr,
            Zeros_8Bit,
            IPv4_Protocol,
            UDP_Len
            }},
    {TCP_PHDR, {
            IPv4_Src_Addr,
            IPv4_Dest_Addr,
            Zeros_8Bit,
            IPv4_Protocol,
            TCP_Total_Len,
            }
    }
};

vector<string> cea_hdr_name = {
    "MAC",
    "LLC",
    "SNAP",
    "VLAN",
    "MPLS",
    "IPv4",
    "IPv6",
    "ARP",
    "TCP",
    "UDP",
    "PAUSE",
    "PFC",
    "UDP_PHDR",
    "TCP_PHDR",
    "META"
};

vector<string> cea_field_name = {
    "MAC_Preamble",
    "MAC_Dest_Addr",
    "MAC_Src_Addr",
    "MAC_Len",
    "MAC_Ether_Type",
    "MAC_Fcs",
    "LLC_Dsap",
    "LLC_Ssap",
    "LLC_Control",
    "SNAP_Oui",
    "SNAP_Pid",
    "IPv4_Version",
    "IPv4_IHL",
    "IPv4_Tos",
    "IPv4_Total_Len",
    "IPv4_Id",
    "IPv4_Flags",
    "IPv4_Frag_Offset",
    "IPv4_TTL",
    "IPv4_Protocol",
    "IPv4_Hdr_Csum",
    "IPv4_Src_Addr",
    "IPv4_Dest_Addr",
    "IPv4_Opts",
    "IPv4_Pad",
    "IPv6_Version",
    "IPv6_Traffic_Class",
    "IPv6_Flow_Label",
    "IPv6_Payload_Len",
    "IPv6_Next_Hdr",
    "IPv6_Hop_Limit",
    "IPv6_Src_Addr",
    "IPv6_Dest_Addr",
    "TCP_Src_Port",
    "TCP_Dest_Port",
    "TCP_Seq_Num",
    "TCP_Ack_Num",
    "TCP_Data_Offset",
    "TCP_Reserved",
    "TCP_Urg",
    "TCP_Ack",
    "TCP_Psh",
    "TCP_Rst",
    "TCP_Syn",
    "TCP_Fin",
    "TCP_Window",
    "TCP_Csum",
    "TCP_Urg_Ptr",
    "TCP_Opts",
    "TCP_Pad",
    "UDP_Src_Port",
    "UDP_Dest_Port",
    "UDP_Len",
    "UDP_Csum",
    "ARP_Hw_Type",
    "ARP_Proto_Type",
    "ARP_Hw_Len",
    "ARP_Proto_Len",
    "ARP_Opcode",
    "ARP_Sender_Hw_Addr",
    "ARP_Sender_Proto_addr",
    "ARP_Target_Hw_Addr",
    "ARP_Target_Proto_Addr",
    "MPLS_Label",
    "MPLS_Exp",
    "MPLS_Stack",
    "MPLS_Ttl",
    "VLAN_Tpi",
    "VLAN_Tci_Pcp",
    "VLAN_Tci_Cfi",
    "VLAN_Vid",
    "MAC_Control",
    "MAC_Control_Opcode",
    "Pause_Quanta",
    "Priority_En_Vector",
    "Pause_Quanta_0",
    "Pause_Quanta_1",
    "Pause_Quanta_2",
    "Pause_Quanta_3",
    "Pause_Quanta_4",
    "Pause_Quanta_5",
    "Pause_Quanta_6",
    "Pause_Quanta_7",
    "FRAME_Len",
    "PAYLOAD_Pattern",
    "STREAM_Traffic_Type",
    "STREAM_Traffic_Control",
    "STREAM_Ipg",
    "STREAM_Isg",
    "STREAM_Ibg",
    "STREAM_Bandwidth",
    "STREAM_Start_Delay",
    "UDF",
    "META_Len",
    "META_Ipg",
    "META_Preamble",
    "META_Pad1",
    "META_Pad2",
    "META_Pad3",
    "META_Pad4",
    "META_Pad5",
    "META_Pad6",
    "Zeros_8Bit",
    "TCP_Total_Len"
};

//------------------------------------------------------------------------------
// Header Class
//------------------------------------------------------------------------------
class cea_header {
public:
    cea_header(cea_hdr_type hdr);
    ~cea_header();

private:
    class core;
    unique_ptr<core> impl;
};

//------------------------------------------------------------------------------
// Header Class Core
//------------------------------------------------------------------------------
class cea_header::core {
public:
    core(cea_hdr_type hdr);
    ~core();
    cea_hdr_type hdr;
    vector<cea_field_id> ids;
    void gen();
};

vector<cea_field> flds = {
{	false	,	0	,	MAC_Preamble               	,	64	,	0	,	Fixed_Value     	,	0x55555555555555d5  	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MAC_Preamble               "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	MAC_Dest_Addr              	,	48	,	0	,	Fixed_Pattern   	,	0                   	,	"00:00:00:00:00:00"    	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MAC_Dest_Addr              "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	MAC_Src_Addr               	,	48	,	0	,	Fixed_Pattern   	,	0                   	,	"00:00:00:00:00:00"    	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MAC_Src_Addr               "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	MAC_Len                    	,	16	,	0	,	Fixed_Value     	,	46                  	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MAC_Len                    "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	MAC_Ether_Type             	,	16	,	0	,	Fixed_Value     	,	0x0800              	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MAC_Ether_Type             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	MAC_Fcs                    	,	32	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MAC_Fcs                    "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	LLC_Dsap                   	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"LLC_Dsap                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	LLC_Ssap                   	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"LLC_Ssap                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	LLC_Control                	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"LLC_Control                "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	SNAP_Oui                   	,	24	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"SNAP_Oui                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	SNAP_Pid                   	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"SNAP_Pid                   "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	IPv4_Version               	,	4	,	0	,	Fixed_Value     	,	4                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Version               "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	IPv4_IHL                   	,	4	,	0	,	Fixed_Value     	,	5                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_IHL                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv4_Tos                   	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Tos                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv4_Total_Len             	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Total_Len             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv4_Id                    	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Id                    "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	IPv4_Flags                 	,	3	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Flags                 "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	IPv4_Frag_Offset           	,	13	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Frag_Offset           "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv4_TTL                   	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_TTL                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv4_Protocol              	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Protocol              "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv4_Hdr_Csum              	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Hdr_Csum              "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv4_Src_Addr              	,	32	,	0	,	Fixed_Pattern   	,	0                   	,	"0.0.0.0"              	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Src_Addr              "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv4_Dest_Addr             	,	32	,	0	,	Fixed_Pattern   	,	0                   	,	"0.0.0.0"              	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Dest_Addr             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv4_Opts                  	,	0	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Opts                  "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv4_Pad                   	,	0	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv4_Pad                   "	,	Integer	,	{}	,	{}	},
{	false	,	2	,	IPv6_Version               	,	4	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv6_Version               "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	IPv6_Traffic_Class         	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv6_Traffic_Class         "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	IPv6_Flow_Label            	,	20	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv6_Flow_Label            "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv6_Payload_Len           	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv6_Payload_Len           "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv6_Next_Hdr              	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv6_Next_Hdr              "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv6_Hop_Limit             	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv6_Hop_Limit             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv6_Src_Addr              	,	128	,	0	,	Fixed_Pattern   	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv6_Src_Addr              "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	IPv6_Dest_Addr             	,	128	,	0	,	Fixed_Pattern   	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"IPv6_Dest_Addr             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	TCP_Src_Port               	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Src_Port               "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	TCP_Dest_Port              	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Dest_Port              "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	TCP_Seq_Num                	,	32	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Seq_Num                "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	TCP_Ack_Num                	,	32	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Ack_Num                "	,	Integer	,	{}	,	{}	},
{	false	,	7	,	TCP_Data_Offset            	,	4	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Data_Offset            "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	TCP_Reserved               	,	6	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Reserved               "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	TCP_Urg                    	,	1	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Urg                    "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	TCP_Ack                    	,	1	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Ack                    "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	TCP_Psh                    	,	1	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Psh                    "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	TCP_Rst                    	,	1	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Rst                    "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	TCP_Syn                    	,	1	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Syn                    "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	TCP_Fin                    	,	1	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Fin                    "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	TCP_Window                 	,	16	,	0	,	Fixed_Value     	,	64                  	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Window                 "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	TCP_Csum                   	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Csum                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	TCP_Urg_Ptr                	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Urg_Ptr                "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	TCP_Opts                   	,	0	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Opts                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	TCP_Pad                    	,	0	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Pad                    "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	UDP_Src_Port               	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"UDP_Src_Port               "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	UDP_Dest_Port              	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"UDP_Dest_Port              "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	UDP_Len                    	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"UDP_Len                    "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	UDP_Csum                   	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"UDP_Csum                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	ARP_Hw_Type                	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"ARP_Hw_Type                "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	ARP_Proto_Type             	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"ARP_Proto_Type             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	ARP_Hw_Len                 	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"ARP_Hw_Len                 "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	ARP_Proto_Len              	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"ARP_Proto_Len              "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	ARP_Opcode                 	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"ARP_Opcode                 "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	ARP_Sender_Hw_Addr         	,	48	,	0	,	Fixed_Pattern   	,	0                   	,	"00:00:00:00:00:00"    	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"ARP_Sender_Hw_Addr         "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	ARP_Sender_Proto_addr      	,	32	,	0	,	Fixed_Pattern   	,	0                   	,	"0.0.0.0"              	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"ARP_Sender_Proto_addr      "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	ARP_Target_Hw_Addr         	,	48	,	0	,	Fixed_Pattern   	,	0                   	,	"00:00:00:00:00:00"    	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"ARP_Target_Hw_Addr         "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	ARP_Target_Proto_Addr      	,	32	,	0	,	Fixed_Pattern   	,	0                   	,	"0.0.0.0"              	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"ARP_Target_Proto_Addr      "	,	Integer	,	{}	,	{}	},
{	false	,	2	,	MPLS_Label                 	,	20	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MPLS_Label                 "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	MPLS_Exp                   	,	3	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MPLS_Exp                   "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	MPLS_Stack                 	,	1	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MPLS_Stack                 "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	MPLS_Ttl                   	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MPLS_Ttl                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	VLAN_Tpi                   	,	16	,	0	,	Fixed_Value     	,	0x8100              	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"VLAN_Tpi                   "	,	Integer	,	{}	,	{}	},
{	false	,	2	,	VLAN_Tci_Pcp               	,	3	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"VLAN_Tci_Pcp               "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	VLAN_Tci_Cfi               	,	1	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"VLAN_Tci_Cfi               "	,	Integer	,	{}	,	{}	},
{	false	,	1	,	VLAN_Vid                   	,	12	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"VLAN_Vid                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	MAC_Control                	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MAC_Control                "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	MAC_Control_Opcode         	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"MAC_Control_Opcode         "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Pause_Quanta               	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Pause_Quanta               "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Priority_En_Vector         	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Priority_En_Vector         "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Pause_Quanta_0             	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Pause_Quanta_0             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Pause_Quanta_1             	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Pause_Quanta_1             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Pause_Quanta_2             	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Pause_Quanta_2             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Pause_Quanta_3             	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Pause_Quanta_3             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Pause_Quanta_4             	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Pause_Quanta_4             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Pause_Quanta_5             	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Pause_Quanta_5             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Pause_Quanta_6             	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Pause_Quanta_6             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Pause_Quanta_7             	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Pause_Quanta_7             "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	FRAME_Len                  	,	32	,	0	,	Fixed_Value     	,	64                  	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"FRAME_Len                  "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	PAYLOAD_Pattern            	,	0	,	0	,	Fixed_Value     	,	0                   	,	"00"                   	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"PAYLOAD_Pattern            "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	STREAM_Traffic_Type        	,	32	,	0	,	Fixed_Value     	,	Continuous          	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"STREAM_Traffic_Type        "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	STREAM_Traffic_Control     	,	32	,	0	,	Fixed_Value     	,	Stop_After_Stream   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"STREAM_Traffic_Control     "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	STREAM_Ipg                 	,	32	,	0	,	Fixed_Value     	,	12                  	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"STREAM_Ipg                 "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	STREAM_Isg                 	,	32	,	0	,	Fixed_Value     	,	12                  	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"STREAM_Ifg                 "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	STREAM_Ibg                 	,	32	,	0	,	Fixed_Value     	,	12                  	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"STREAM_Ibg                 "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	STREAM_Bandwidth           	,	32	,	0	,	Fixed_Value     	,	100                 	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"STREAM_Bandwidth           "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	STREAM_Start_Delay         	,	32	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"STREAM_Start_Delay         "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	UDF                        	,	0	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"UDF                        "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	META_Len                   	,	32	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"META_Len                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	META_Ipg                   	,	32	,	0	,	Fixed_Value     	,	12                  	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"META_Ipg                   "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	META_Preamble              	,	64	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"META_Preamble              "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	META_Pad1                  	,	64	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"META_Pad1                  "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	META_Pad2                  	,	64	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"META_Pad2                  "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	META_Pad3                  	,	64	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"META_Pad3                  "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	META_Pad4                  	,	64	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"META_Pad4                  "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	META_Pad5                  	,	64	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"META_Pad5                  "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	META_Pad6                  	,	64	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"META_Pad6                  "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	Zeros_8Bit                 	,	8	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"Zeros_8Bit                 "	,	Integer	,	{}	,	{}	},
{	false	,	0	,	TCP_Total_Len              	,	16	,	0	,	Fixed_Value     	,	0                   	,	""                     	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	0	,	"TCP_Total_Len              "	,	Integer	,	{}	,	{}	},
};

} // namespace

struct hdr {
    uint32_t id;
    string name;
    vector<uint32_t> list;
};

void print_vector (vector<string> vec) {
    for (auto x : vec) {
        cout << x << endl;
    }
}

//------------------------------------------------------------------------------
// Main Program
//------------------------------------------------------------------------------

using namespace cea;

int main() {
    cout << "Hello World!" << endl;
    print_vector(cea_field_name);
    return 0;
}
