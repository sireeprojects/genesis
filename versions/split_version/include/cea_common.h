#pragma once

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
    Random_In_Range,
    Weighted_Distribution,
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
        vector<pair<uint64_t, double>> distr;
        string distr_name;
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

}
