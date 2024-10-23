#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <memory>

using namespace std;

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
    Fixed_Pattern,
    Value_List,
    Pattern_List,
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

enum cea_field_type {
    Integer,
    Pattern,
    Pattern_PRE,
    Pattern_MAC,
    Pattern_IPv4,
    Pattern_IPv6
};

enum cea_stream_add_type {
    Header,
    Field
};

struct cea_runtime {
    uint64_t value_in_range;
    uint64_t gen_value;
    unsigned char gen_pattern[16]; // DO NOT CHANGE the position of this array in the struct
    uint32_t cur_count;
    uint32_t list_idx;
};

struct cea_field_spec {
    bool is_mutable;
    uint32_t merge;
    cea_field_id id;
    uint32_t len;
    uint32_t offset;
    bool proto_list_specified;
    bool auto_field;
    string name;
    cea_field_type type;
    cea_gen_spec spec;
    cea_runtime rt;
};

vector<cea_field_spec> fdb = {
{false, 0, MAC_Preamble          , 64 , 0, 0, 0, "MAC_Preamble          ", Pattern_PRE,  { Fixed_Pattern, 0                , "55555555555555d"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, MAC_Dest_Addr         , 48 , 0, 0, 0, "MAC_Dest_Addr         ", Pattern_MAC,  { Fixed_Pattern, 0                , "01:02:03:04:05:06", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, MAC_Src_Addr          , 48 , 0, 0, 0, "MAC_Src_Addr          ", Pattern_MAC,  { Fixed_Pattern, 0                , "0a:0b:0c:0d:0e:0f", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, MAC_Len               , 16 , 0, 0, 0, "MAC_Len               ", Integer,      { Fixed_Value  , 46               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, MAC_Ether_Type        , 16 , 0, 0, 0, "MAC_Ether_Type        ", Integer,      { Fixed_Value  , 0x0800           , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, MAC_Fcs               , 32 , 0, 0, 0, "MAC_Fcs               ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, LLC_Dsap              , 8  , 0, 0, 0, "LLC_Dsap              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, LLC_Ssap              , 8  , 0, 0, 0, "LLC_Ssap              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, LLC_Control           , 8  , 0, 0, 0, "LLC_Control           ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, SNAP_Oui              , 24 , 0, 0, 0, "SNAP_Oui              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, SNAP_Pid              , 16 , 0, 0, 0, "SNAP_Pid              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, IPv4_Version          , 4  , 0, 0, 0, "IPv4_Version          ", Integer,      { Fixed_Value  , 4                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, IPv4_IHL              , 4  , 0, 0, 0, "IPv4_IHL              ", Integer,      { Fixed_Value  , 5                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv4_Tos              , 8  , 0, 0, 0, "IPv4_Tos              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv4_Total_Len        , 16 , 0, 0, 0, "IPv4_Total_Len        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv4_Id               , 16 , 0, 0, 0, "IPv4_Id               ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, IPv4_Flags            , 3  , 0, 0, 0, "IPv4_Flags            ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, IPv4_Frag_Offset      , 13 , 0, 0, 0, "IPv4_Frag_Offset      ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv4_TTL              , 8  , 0, 0, 0, "IPv4_TTL              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv4_Protocol         , 8  , 0, 0, 0, "IPv4_Protocol         ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv4_Hdr_Csum         , 16 , 0, 0, 0, "IPv4_Hdr_Csum         ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv4_Src_Addr         , 32 , 0, 0, 0, "IPv4_Src_Addr         ", Pattern_IPv4, { Fixed_Pattern, 0                , "192.168.0.1"      , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv4_Dest_Addr        , 32 , 0, 0, 0, "IPv4_Dest_Addr        ", Pattern_IPv4, { Fixed_Pattern, 0                , "255.255.255.255"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv4_Opts             , 0  , 0, 0, 0, "IPv4_Opts             ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv4_Pad              , 0  , 0, 0, 0, "IPv4_Pad              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 2, IPv6_Version          , 4  , 0, 0, 0, "IPv6_Version          ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, IPv6_Traffic_Class    , 8  , 0, 0, 0, "IPv6_Traffic_Class    ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, IPv6_Flow_Label       , 20 , 0, 0, 0, "IPv6_Flow_Label       ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv6_Payload_Len      , 16 , 0, 0, 0, "IPv6_Payload_Len      ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv6_Next_Hdr         , 8  , 0, 0, 0, "IPv6_Next_Hdr         ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv6_Hop_Limit        , 8  , 0, 0, 0, "IPv6_Hop_Limit        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv6_Src_Addr         , 128, 0, 0, 0, "IPv6_Src_Addr         ", Pattern_IPv6, { Fixed_Pattern, 0                , "0.0.0.0.0.0.0.0"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, IPv6_Dest_Addr        , 128, 0, 0, 0, "IPv6_Dest_Addr        ", Pattern_IPv6, { Fixed_Pattern, 0                , "0.0.0.0.0.0.0.0"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, TCP_Src_Port          , 16 , 0, 0, 0, "TCP_Src_Port          ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, TCP_Dest_Port         , 16 , 0, 0, 0, "TCP_Dest_Port         ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, TCP_Seq_Num           , 32 , 0, 0, 0, "TCP_Seq_Num           ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, TCP_Ack_Num           , 32 , 0, 0, 0, "TCP_Ack_Num           ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 7, TCP_Data_Offset       , 4  , 0, 0, 0, "TCP_Data_Offset       ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, TCP_Reserved          , 6  , 0, 0, 0, "TCP_Reserved          ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, TCP_Urg               , 1  , 0, 0, 0, "TCP_Urg               ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, TCP_Ack               , 1  , 0, 0, 0, "TCP_Ack               ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, TCP_Psh               , 1  , 0, 0, 0, "TCP_Psh               ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, TCP_Rst               , 1  , 0, 0, 0, "TCP_Rst               ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, TCP_Syn               , 1  , 0, 0, 0, "TCP_Syn               ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, TCP_Fin               , 1  , 0, 0, 0, "TCP_Fin               ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, TCP_Window            , 16 , 0, 0, 0, "TCP_Window            ", Integer,      { Fixed_Value  , 64               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, TCP_Csum              , 16 , 0, 0, 0, "TCP_Csum              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, TCP_Urg_Ptr           , 16 , 0, 0, 0, "TCP_Urg_Ptr           ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, TCP_Opts              , 0  , 0, 0, 0, "TCP_Opts              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, TCP_Pad               , 0  , 0, 0, 0, "TCP_Pad               ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, UDP_Src_Port          , 16 , 0, 0, 0, "UDP_Src_Port          ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, UDP_Dest_Port         , 16 , 0, 0, 0, "UDP_Dest_Port         ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, UDP_Len               , 16 , 0, 0, 0, "UDP_Len               ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, UDP_Csum              , 16 , 0, 0, 0, "UDP_Csum              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, ARP_Hw_Type           , 16 , 0, 0, 0, "ARP_Hw_Type           ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, ARP_Proto_Type        , 16 , 0, 0, 0, "ARP_Proto_Type        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, ARP_Hw_Len            , 8  , 0, 0, 0, "ARP_Hw_Len            ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, ARP_Proto_Len         , 8  , 0, 0, 0, "ARP_Proto_Len         ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, ARP_Opcode            , 16 , 0, 0, 0, "ARP_Opcode            ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, ARP_Sender_Hw_Addr    , 48 , 0, 0, 0, "ARP_Sender_Hw_Addr    ", Pattern_MAC,  { Fixed_Pattern, 0                , "00:00:00:00:00:00", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, ARP_Sender_Proto_addr , 32 , 0, 0, 0, "ARP_Sender_Proto_addr ", Pattern_IPv4, { Fixed_Pattern, 0                , "0.0.0.0"          , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, ARP_Target_Hw_Addr    , 48 , 0, 0, 0, "ARP_Target_Hw_Addr    ", Pattern_MAC,  { Fixed_Pattern, 0                , "00:00:00:00:00:00", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, ARP_Target_Proto_Addr , 32 , 0, 0, 0, "ARP_Target_Proto_Addr ", Pattern_IPv4, { Fixed_Pattern, 0                , "0.0.0.0"          , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 2, MPLS_Label            , 20 , 0, 0, 0, "MPLS_Label            ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, MPLS_Exp              , 3  , 0, 0, 0, "MPLS_Exp              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, MPLS_Stack            , 1  , 0, 0, 0, "MPLS_Stack            ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, MPLS_Ttl              , 8  , 0, 0, 0, "MPLS_Ttl              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, VLAN_Tpi              , 16 , 0, 0, 0, "VLAN_Tpi              ", Integer,      { Fixed_Value  , 0x8100           , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 2, VLAN_Tci_Pcp          , 3  , 0, 0, 0, "VLAN_Tci_Pcp          ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, VLAN_Tci_Cfi          , 1  , 0, 0, 0, "VLAN_Tci_Cfi          ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 1, VLAN_Vid              , 12 , 0, 0, 0, "VLAN_Vid              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, MAC_Control           , 16 , 0, 0, 0, "MAC_Control           ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, MAC_Control_Opcode    , 16 , 0, 0, 0, "MAC_Control_Opcode    ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Pause_Quanta          , 16 , 0, 0, 0, "Pause_Quanta          ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Priority_En_Vector    , 16 , 0, 0, 0, "Priority_En_Vector    ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Pause_Quanta_0        , 16 , 0, 0, 0, "Pause_Quanta_0        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Pause_Quanta_1        , 16 , 0, 0, 0, "Pause_Quanta_1        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Pause_Quanta_2        , 16 , 0, 0, 0, "Pause_Quanta_2        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Pause_Quanta_3        , 16 , 0, 0, 0, "Pause_Quanta_3        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Pause_Quanta_4        , 16 , 0, 0, 0, "Pause_Quanta_4        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Pause_Quanta_5        , 16 , 0, 0, 0, "Pause_Quanta_5        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Pause_Quanta_6        , 16 , 0, 0, 0, "Pause_Quanta_6        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Pause_Quanta_7        , 16 , 0, 0, 0, "Pause_Quanta_7        ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, FRAME_Len             , 32 , 0, 0, 0, "FRAME_Len             ", Integer,      { Fixed_Value  , 64               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, PAYLOAD_Pattern       , 0  , 0, 0, 0, "PAYLOAD_Pattern       ", Integer,      { Fixed_Pattern, 0                , "00"               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, STREAM_Traffic_Type   , 32 , 0, 0, 0, "STREAM_Traffic_Type   ", Integer,      { Fixed_Value  , Continuous       , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, STREAM_Traffic_Control, 32 , 0, 0, 0, "STREAM_Traffic_Control", Integer,      { Fixed_Value  , Stop_After_Stream, ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, STREAM_Ipg            , 32 , 0, 0, 0, "STREAM_Ipg            ", Integer,      { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, STREAM_Isg            , 32 , 0, 0, 0, "STREAM_Ifg            ", Integer,      { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, STREAM_Ibg            , 32 , 0, 0, 0, "STREAM_Ibg            ", Integer,      { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, STREAM_Bandwidth      , 32 , 0, 0, 0, "STREAM_Bandwidth      ", Integer,      { Fixed_Value  , 100              , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, STREAM_Start_Delay    , 32 , 0, 0, 0, "STREAM_Start_Delay    ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, UDF                   , 0  , 0, 0, 0, "UDF                   ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, META_Len              , 32 , 0, 0, 0, "META_Len              ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, META_Ipg              , 32 , 0, 0, 0, "META_Ipg              ", Integer,      { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, META_Preamble         , 64 , 0, 0, 0, "META_Preamble         ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, META_Pad1             , 64 , 0, 0, 0, "META_Pad1             ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, META_Pad2             , 64 , 0, 0, 0, "META_Pad2             ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, META_Pad3             , 64 , 0, 0, 0, "META_Pad3             ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, META_Pad4             , 64 , 0, 0, 0, "META_Pad4             ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, META_Pad5             , 64 , 0, 0, 0, "META_Pad5             ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, META_Pad6             , 64 , 0, 0, 0, "META_Pad6             ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, Zeros_8Bit            , 8  , 0, 0, 0, "Zeros_8Bit            ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
{false, 0, TCP_Total_Len         , 16 , 0, 0, 0, "TCP_Total_Len         ", Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'}, 0, 0 }},
};

