/*
Pending features   
-----------------
- ethertype/len field is not yet handled properly
- support for vlan/mpls etc
  - do it after mutate starts working
- implement reset() for all classes
- make sure to release temp memory allocation
- add API documenation in cea.h file
- multi process support (stream_id and port_id)
- multi reset support
- multi iteration support
- multi queue support (CoS, QoS)
- during multi reset/iteration, what if user wants to update only 
  certain field/header/property. Do I need to add API to update
  stream field/header/property? Or the user needs to reset the stream
  and set the fiels/header/properties again.
  - test.cpp creates headers as pointers, hence the user needs to be
    able to reuse those and not create another pointer
  - add support to update the stream (field/header/property)
- add support to define a stream in a single API
- add support for avip style test case

Notes to improve performance
-----------------------------
- check if we can allocate large memories in NUMA, hugepage memory
*/  

#include <thread>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <string>
#include <map>
#include <cmath>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <unistd.h>
#include <chrono>
#include <cassert>
#include <random>
#include <csignal>
#include <regex>
#include "cea.h"

using namespace std;
using namespace chrono;

#define CEA_PACKED __attribute__((packed))

// maximum supported frame size from MAC dest addr to MAC crc (16KB)
#define CEA_MAX_FRAME_SIZE 16384

// frame metadata (control header) size in bytes
#define CEA_FRM_METASIZE 64

// total length of '-' used by cea_formatted_hdr
#define CEA_FORMATTED_HDR_LEN 80

#define CEA_MAX_RND_ARRAYS 16
#define CEA_RND_ARRAY_SIZE 1000000  // 1M
#define CEA_PF_SIZE 1000000  // 1M

// CEA_MSG() - Used for mandatory messages inside classes.
// Cannot be disabled in debug mode
#define CEA_MSG(msg) { \
    stringstream s; \
    s << msg; cealog << "(" << msg_prefix << "|" << setw(8) << left \
    << string(__FUNCTION__) << ")" << ": " <<  s.str() \
    << endl; \
}

// CEA_ERR_MSG() - Used for mandatory error messages inside classes.
// Cannot be disabled in debug mode
#define CEA_ERR_MSG(msg) { \
    stringstream s; \
    s << msg; \
    cealog << endl << cea_formatted_hdr("Fatal Error"); \
    cealog << "(" << msg_prefix << "|" << string(__FUNCTION__) << ")" \
    << ": " <<  s.str() << endl; \
    cealog << string(CEA_FORMATTED_HDR_LEN, '-') << endl; \
}

// CEA_DBG() - Enabled only in debug mode
#ifdef CEA_DEBUG
    #define CEA_DBG(msg) { CEA_MSG(msg) }
#else
    #define CEA_DBG(msg) {}
#endif

// Flags to create 1GB Hugepage for frame buffer (shared memory)
#define LENGTH (1UL*1024*1024*1024)
#define PROTECTION (PROT_READ | PROT_WRITE)
#define ADDR (void *)(0x0UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)

namespace cea {

string msg_prefix = "cea";    

// global variable to track proxy and stream id
// TODO This will become a problem in multi-process mode
uint32_t stream_id = 0;
uint32_t port_id = 0;

// regex pattern to check user inputs
regex regex_mac("([[:xdigit:]]{2}[:]?){5}[[:xdigit:]]{2}");
regex regex_ipv4("^(?:(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])[.]){3}(?:25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9]?[0-9])$");
regex regex_ipv6("((([0-9a-fA-F]){1,4})[:]){7}([0-9a-fA-F]){1,4}");
regex regex_pre("[[:xdigit:]]{16}");

enum cea_field_type {
    Integer,
    Pattern_PRE,
    Pattern_MAC,
    Pattern_IPv4,
    Pattern_IPv6
};

enum cea_stream_add_type {
    Header,
    Field
};

struct cea_field_runtime {
    uint64_t value;
    vector<uint64_t> patterns;
    uint32_t count;
    uint32_t idx;
};

struct cea_field_spec {
    uint32_t merge;
    cea_field_id id;
    uint32_t len;
    bool proto_list_specified;
    bool auto_field;
    string name;
    uint64_t def_value;
    vector <unsigned char> def_pattern;
    cea_field_type type;
    cea_field_genspec spec;
    cea_field_runtime rt;
};

struct cea_field_mutation_data {
    bool is_mutable;
    uint32_t offset;
};
 
struct cea_field_mutation_spec {
    cea_field_spec defaults;
    cea_field_genspec gspec;
    cea_field_runtime rt;
    cea_field_mutation_data mdata;
};

vector<unsigned char>def_pre_pattern    = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x5d};
vector<unsigned char>def_dstmac_pattern = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
vector<unsigned char>def_srcmac_pattern = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
vector<unsigned char>def_srcip4_pattern = {0xc0, 0xa8, 0x00, 0x01};
vector<unsigned char>def_dstip4_pattern = {0xff, 0xff, 0xff, 0xff};
vector<unsigned char>def_srcip6_pattern = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
vector<unsigned char>def_dstip6_pattern = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

vector<cea_field_mutation_spec> mtable = {
{{0, MAC_Preamble          , 64 , 0, 0, "MAC_Preamble          ", 0                , def_pre_pattern    , Pattern_PRE }, { Fixed_Pattern, 0                , "55555555555555d"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, MAC_Dest_Addr         , 48 , 0, 0, "MAC_Dest_Addr         ", 0                , def_dstmac_pattern , Pattern_MAC }, { Fixed_Pattern, 0                , "01:02:03:04:05:06", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, MAC_Src_Addr          , 48 , 0, 0, "MAC_Src_Addr          ", 0                , def_srcmac_pattern , Pattern_MAC }, { Fixed_Pattern, 0                , "0a:0b:0c:0d:0e:0f", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, MAC_Len               , 16 , 0, 0, "MAC_Len               ", 46               , {0x00}             , Integer     }, { Fixed_Value  , 46               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, MAC_Ether_Type        , 16 , 0, 0, "MAC_Ether_Type        ", 0x0800           , {0x00}             , Integer     }, { Fixed_Value  , 0x0800           , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, MAC_Fcs               , 32 , 0, 0, "MAC_Fcs               ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, LLC_Dsap              , 8  , 0, 0, "LLC_Dsap              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, LLC_Ssap              , 8  , 0, 0, "LLC_Ssap              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, LLC_Control           , 8  , 0, 0, "LLC_Control           ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, SNAP_Oui              , 24 , 0, 0, "SNAP_Oui              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, SNAP_Pid              , 16 , 0, 0, "SNAP_Pid              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, IPv4_Version          , 4  , 0, 0, "IPv4_Version          ", 4                , {0x00}             , Integer     }, { Fixed_Value  , 4                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, IPv4_IHL              , 4  , 0, 0, "IPv4_IHL              ", 5                , {0x00}             , Integer     }, { Fixed_Value  , 5                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv4_Tos              , 8  , 0, 0, "IPv4_Tos              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv4_Total_Len        , 16 , 0, 0, "IPv4_Total_Len        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv4_Id               , 16 , 0, 0, "IPv4_Id               ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, IPv4_Flags            , 3  , 0, 0, "IPv4_Flags            ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, IPv4_Frag_Offset      , 13 , 0, 0, "IPv4_Frag_Offset      ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv4_TTL              , 8  , 0, 0, "IPv4_TTL              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv4_Protocol         , 8  , 0, 0, "IPv4_Protocol         ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv4_Hdr_Csum         , 16 , 0, 0, "IPv4_Hdr_Csum         ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv4_Src_Addr         , 32 , 0, 0, "IPv4_Src_Addr         ", 0                , def_srcip4_pattern , Pattern_IPv4}, { Fixed_Pattern, 0                , "192.168.0.1"      , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv4_Dest_Addr        , 32 , 0, 0, "IPv4_Dest_Addr        ", 0                , def_dstip4_pattern , Pattern_IPv4}, { Fixed_Pattern, 0                , "255.255.255.255"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv4_Opts             , 0  , 0, 0, "IPv4_Opts             ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv4_Pad              , 0  , 0, 0, "IPv4_Pad              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{2, IPv6_Version          , 4  , 0, 0, "IPv6_Version          ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, IPv6_Traffic_Class    , 8  , 0, 0, "IPv6_Traffic_Class    ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, IPv6_Flow_Label       , 20 , 0, 0, "IPv6_Flow_Label       ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv6_Payload_Len      , 16 , 0, 0, "IPv6_Payload_Len      ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv6_Next_Hdr         , 8  , 0, 0, "IPv6_Next_Hdr         ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv6_Hop_Limit        , 8  , 0, 0, "IPv6_Hop_Limit        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv6_Src_Addr         , 128, 0, 0, "IPv6_Src_Addr         ", 0                , def_srcip6_pattern , Pattern_IPv6}, { Fixed_Pattern, 0                , "0.0.0.0.0.0.0.0"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, IPv6_Dest_Addr        , 128, 0, 0, "IPv6_Dest_Addr        ", 0                , def_dstip6_pattern , Pattern_IPv6}, { Fixed_Pattern, 0                , "0.0.0.0.0.0.0.0"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, TCP_Src_Port          , 16 , 0, 0, "TCP_Src_Port          ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, TCP_Dest_Port         , 16 , 0, 0, "TCP_Dest_Port         ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, TCP_Seq_Num           , 32 , 0, 0, "TCP_Seq_Num           ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, TCP_Ack_Num           , 32 , 0, 0, "TCP_Ack_Num           ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{7, TCP_Data_Offset       , 4  , 0, 0, "TCP_Data_Offset       ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, TCP_Reserved          , 6  , 0, 0, "TCP_Reserved          ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, TCP_Urg               , 1  , 0, 0, "TCP_Urg               ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, TCP_Ack               , 1  , 0, 0, "TCP_Ack               ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, TCP_Psh               , 1  , 0, 0, "TCP_Psh               ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, TCP_Rst               , 1  , 0, 0, "TCP_Rst               ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, TCP_Syn               , 1  , 0, 0, "TCP_Syn               ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, TCP_Fin               , 1  , 0, 0, "TCP_Fin               ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, TCP_Window            , 16 , 0, 0, "TCP_Window            ", 64               , {0x00}             , Integer     }, { Fixed_Value  , 64               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, TCP_Csum              , 16 , 0, 0, "TCP_Csum              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, TCP_Urg_Ptr           , 16 , 0, 0, "TCP_Urg_Ptr           ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, TCP_Opts              , 0  , 0, 0, "TCP_Opts              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, TCP_Pad               , 0  , 0, 0, "TCP_Pad               ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, UDP_Src_Port          , 16 , 0, 0, "UDP_Src_Port          ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, UDP_Dest_Port         , 16 , 0, 0, "UDP_Dest_Port         ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, UDP_Len               , 16 , 0, 0, "UDP_Len               ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, UDP_Csum              , 16 , 0, 0, "UDP_Csum              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, ARP_Hw_Type           , 16 , 0, 0, "ARP_Hw_Type           ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, ARP_Proto_Type        , 16 , 0, 0, "ARP_Proto_Type        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, ARP_Hw_Len            , 8  , 0, 0, "ARP_Hw_Len            ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, ARP_Proto_Len         , 8  , 0, 0, "ARP_Proto_Len         ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, ARP_Opcode            , 16 , 0, 0, "ARP_Opcode            ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, ARP_Sender_Hw_Addr    , 48 , 0, 0, "ARP_Sender_Hw_Addr    ", 0                , def_srcmac_pattern , Pattern_MAC }, { Fixed_Pattern, 0                , "00:00:00:00:00:00", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, ARP_Sender_Proto_addr , 32 , 0, 0, "ARP_Sender_Proto_addr ", 0                , def_srcip4_pattern , Pattern_IPv4}, { Fixed_Pattern, 0                , "0.0.0.0"          , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, ARP_Target_Hw_Addr    , 48 , 0, 0, "ARP_Target_Hw_Addr    ", 0                , def_dstmac_pattern , Pattern_MAC }, { Fixed_Pattern, 0                , "00:00:00:00:00:00", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, ARP_Target_Proto_Addr , 32 , 0, 0, "ARP_Target_Proto_Addr ", 0                , def_dstip4_pattern , Pattern_IPv4}, { Fixed_Pattern, 0                , "0.0.0.0"          , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{2, MPLS_Label            , 20 , 0, 0, "MPLS_Label            ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, MPLS_Exp              , 3  , 0, 0, "MPLS_Exp              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, MPLS_Stack            , 1  , 0, 0, "MPLS_Stack            ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, MPLS_Ttl              , 8  , 0, 0, "MPLS_Ttl              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, VLAN_Tpi              , 16 , 0, 0, "VLAN_Tpi              ", 0x8100           , {0x00}             , Integer     }, { Fixed_Value  , 0x8100           , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{2, VLAN_Tci_Pcp          , 3  , 0, 0, "VLAN_Tci_Pcp          ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, VLAN_Tci_Cfi          , 1  , 0, 0, "VLAN_Tci_Cfi          ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{1, VLAN_Vid              , 12 , 0, 0, "VLAN_Vid              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, MAC_Control           , 16 , 0, 0, "MAC_Control           ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, MAC_Control_Opcode    , 16 , 0, 0, "MAC_Control_Opcode    ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Pause_Quanta          , 16 , 0, 0, "Pause_Quanta          ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Priority_En_Vector    , 16 , 0, 0, "Priority_En_Vector    ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Pause_Quanta_0        , 16 , 0, 0, "Pause_Quanta_0        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Pause_Quanta_1        , 16 , 0, 0, "Pause_Quanta_1        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Pause_Quanta_2        , 16 , 0, 0, "Pause_Quanta_2        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Pause_Quanta_3        , 16 , 0, 0, "Pause_Quanta_3        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Pause_Quanta_4        , 16 , 0, 0, "Pause_Quanta_4        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Pause_Quanta_5        , 16 , 0, 0, "Pause_Quanta_5        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Pause_Quanta_6        , 16 , 0, 0, "Pause_Quanta_6        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Pause_Quanta_7        , 16 , 0, 0, "Pause_Quanta_7        ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, FRAME_Len             , 32 , 0, 0, "FRAME_Len             ", 64               , {0x00}             , Integer     }, { Fixed_Value  , 64               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, PAYLOAD_Pattern       , 0  , 0, 0, "PAYLOAD_Pattern       ", 0                , {0x00}             , Integer     }, { Fixed_Pattern, 0                , "00"               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, STREAM_Traffic_Type   , 32 , 0, 0, "STREAM_Traffic_Type   ", Continuous       , {0x00}             , Integer     }, { Fixed_Value  , Continuous       , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, STREAM_Traffic_Control, 32 , 0, 0, "STREAM_Traffic_Control", Stop_After_Stream, {0x00}             , Integer     }, { Fixed_Value  , Stop_After_Stream, ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, STREAM_Ipg            , 32 , 0, 0, "STREAM_Ipg            ", 12               , {0x00}             , Integer     }, { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, STREAM_Isg            , 32 , 0, 0, "STREAM_Ifg            ", 12               , {0x00}             , Integer     }, { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, STREAM_Ibg            , 32 , 0, 0, "STREAM_Ibg            ", 12               , {0x00}             , Integer     }, { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, STREAM_Bandwidth      , 32 , 0, 0, "STREAM_Bandwidth      ", 100              , {0x00}             , Integer     }, { Fixed_Value  , 100              , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, STREAM_Start_Delay    , 32 , 0, 0, "STREAM_Start_Delay    ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, UDF                   , 0  , 0, 0, "UDF                   ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, META_Len              , 32 , 0, 0, "META_Len              ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, META_Ipg              , 32 , 0, 0, "META_Ipg              ", 12               , {0x00}             , Integer     }, { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, META_Preamble         , 64 , 0, 0, "META_Preamble         ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, META_Pad1             , 64 , 0, 0, "META_Pad1             ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, META_Pad2             , 64 , 0, 0, "META_Pad2             ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, META_Pad3             , 64 , 0, 0, "META_Pad3             ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, META_Pad4             , 64 , 0, 0, "META_Pad4             ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, META_Pad5             , 64 , 0, 0, "META_Pad5             ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, META_Pad6             , 64 , 0, 0, "META_Pad6             ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, Zeros_8Bit            , 8  , 0, 0, "Zeros_8Bit            ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
{{0, TCP_Total_Len         , 16 , 0, 0, "TCP_Total_Len         ", 0                , {0x00}             , Integer     }, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }, {}},
};

void signal_handler(int signal) {
    if (signal == SIGABRT) {
        cealog << endl << "<<<< Aborting simulation >>>>" << endl;
    } else {
        cerr << "Unexpected signal " << signal << " received\n";
    }
    exit(EXIT_FAILURE);
}

// map header type to list of associated fields
map <cea_header_type, vector <cea_field_id>> header_to_field_map = {
    {PROPERTIES, {
             FRAME_Len,
             PAYLOAD_Pattern,
             STREAM_Traffic_Type,
             STREAM_Traffic_Control,
             STREAM_Ipg,
             STREAM_Isg,
             STREAM_Ibg,
             STREAM_Bandwidth,
             STREAM_Start_Delay
            }},
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

vector<string> cea_header_name = {
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
    "META",
    "PROPERTIES"
};

vector<string> cea_gen_type_name = {
    "Fixed_Value",
    "Fixed_Pattern",
    "Value_List",
    "Pattern_List",
    "Increment",
    "Decrement",
    "Random",
    "Increment_Byte",
    "Decrement_Byte",
    "Increment_Word",
    "Decrement_Word",
    "Continuous",
    "Bursty",
    "Stop_After_Stream",
    "Goto_Next_Stream"
};

vector<string> cea_field_type_name = {
    "Integer",
    "Pattern",
    "Pattern_PRE",
    "Pattern_MAC",
    "Pattern_IPv4",
    "Pattern_IPv6"
};

vector<string> cea_stream_feature_name = {
    "PCAP_Record_Tx_Enable",
    "PCAP_Record_Rx_Enable"
};

// file stream for cea message logging
ofstream logfile;

// custom output stream to sent messages to both screen and log file    
int outbuf::overflow(int_type c) {
    if (c != EOF) {
    c = static_cast<char>(c);
        logfile << static_cast<char>(c);
        if (putchar(c) == EOF) {
           return EOF;
        }
    }
    return c;
}

//------------------------------------------------------------------------------
// cea library init
//------------------------------------------------------------------------------

class cea_init {
public:
    cea_init() {
        signal(SIGABRT, signal_handler);
        logfile.open("run.log", ofstream::out);
        if (!logfile.is_open()) {

            exit(1);
        }
    }
    ~cea_init() { logfile.close(); }
};
cea_init init;

// utility
bool in_range(uint32_t low, uint32_t high, uint32_t x) {        
    return (low <= x && x <= high);         
}

// memcpy in network byte order
void *cea_memcpy_ntw_byte_order(void *dest, const void *src, size_t len) {
    if (len==0) return dest;
    char *d = (char*)dest;
    const char *s = (char*)src;
    int i = 0;
    for (i=(len-1); i>=0; i--) {
        *d++ = s[i];
    }
    return dest;
}

// reverse the order of bytes 
uint64_t reverse_byte_order(uint64_t original, uint32_t num) {
   uint64_t reversed = 0;
   uint32_t loopVar;
   for (loopVar=0; loopVar<num; loopVar++) {
      reversed = (reversed <<8) | (original & 0xFF);
      original = original >> 8;
   }
   return reversed;
}

// remove trailing whitespaces from a string
string cea_rtrim(string s) {
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    return s;
}

// remove leading whitespaces from a string
string cea_ltrim(string s) {
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
    return s;
}

// remove leading and trailing whitespaces from a string
string cea_trim(string s) {
    return cea_ltrim(cea_rtrim(s));
}

//  break a long string to a multiline string
string cea_wordwrap(string msg, uint32_t line_width=100) {
    stringstream wrapped_msg;
    string leading_spaces = "    ";
    for (uint32_t pos=0; pos<msg.length(); pos=pos+line_width) {
        wrapped_msg << leading_spaces
        << cea_ltrim(msg.substr(pos, line_width)) << endl;
    }
    return wrapped_msg.str();
}

string cea_formatted_hdr(string s) {
    stringstream ss;
    ss.setf(ios_base::left);
    ss << string(3,'-') << "{ ";
    ss << s << " }" << string((CEA_FORMATTED_HDR_LEN-(7+s.length())),'-');
    ss << endl;
    return ss.str();
}

// read a environment variable and return stringize
string cea_getenv(string &key) {
    char *val = getenv(key.c_str());
    return (val==NULL) ? string() : string(val);
}

enum cea_readable_type {
    KIS1024 = 1024,
    KIS1000 = 1000
};

// convert a number to its equivalent in KB, MB, GB etc
// type determines if K (kilo) should be considered 
// as 1000 or 1024. default is 1024
string cea_readable_fs(double size, cea_readable_type type) {
    int i = 0;
    ostringstream buf("");
    const char *units[] = {"B", "K", "M", "G", "T", "P", "E", "Z", "Y"};
    int divider = type;

    while (size > (divider-1)) {
      size /= divider;
      i++;
    }
    buf << fixed << setprecision(0) << size << " " << units[i]; 
    return buf.str();
}

//------------------------------------------------------------------------------
// Timer class for runtime performance measurement
//------------------------------------------------------------------------------

class cea_timer {
public:
    cea_timer() = default;
    void start();
    double elapsed();
    string elapsed_in_string(int precision=9);
private:
    time_point<high_resolution_clock> begin;
    time_point<high_resolution_clock> end;
};

void cea_timer::start() {
   begin = high_resolution_clock::now();
}

double cea_timer::elapsed() {
    end = high_resolution_clock::now();
    double delta = duration<double>(end-begin).count();
    return delta;
}

string cea_timer::elapsed_in_string(int precision) {
    stringstream ss;
    ss << fixed << setprecision(precision) << elapsed() << " sec";
    return ss.str();
}

void print_uchar_array (unsigned char* tmp, int len) {
    stringstream s;
    s.str("");
    uint32_t idx = 0;

    for (int x=0; x<len/16; x++) {
        for (int y=0; y<16; y++) {
            s << noshowbase << setw(2) << setfill('0')
              << hex << uint16_t(tmp[idx]) << " ";
            if (y == 7 )
                s << " ";
            idx++;
        }
        s << endl;
    }
    int spacer = 0;
    for (int x=idx; x<len; x++) {
       s<<noshowbase<<setw(2)<<setfill('0') <<hex<<uint16_t(tmp[idx])<<" ";
       idx++;
       spacer++;
       if (spacer == 8) s << " ";
    }
    cealog << "Array Data " << string(37, '-') << endl << s.str()<<endl << string(48, '-') << endl;
    fflush (stdout);
}

//------------------------------------------------------------------------------
// support for PCAP write
//------------------------------------------------------------------------------

class pcap {
public:
    // ctor
    pcap(string filename, string name, uint32_t id);

    // dtor
    ~pcap();

    // write the given buffer into the pcap file
    void write(unsigned char *buf, uint32_t len);

    // name of the pcap file to create
    string pcap_filename;

    // handle to the pcap file
    ofstream pcapfile;

    bool file_exists(const string &filename);

    // set when the object is created
    string parent_name;

    // automatically assigned when the proxy object is created
    // the value of the field is set from the global variable port_id
    uint32_t parent_id;

    // build a string to be prefixed in all messages generated from this class
    string msg_prefix;

    // pcap global file header
    struct CEA_PACKED pcap_file_hdr {
        uint32_t magic : 32;
        uint16_t version_major : 16;
        uint16_t version_minor : 16;
        uint32_t thiszone : 32;
        uint32_t sigfigs : 32;
        uint32_t snaplen : 32;
        uint32_t linktype : 32;
    } fh;
    
    // pcap per frame header
    struct CEA_PACKED pcap_pkt_hdr {
        uint32_t tv_sec : 32;
        uint32_t tv_usec : 32;
        uint32_t caplen : 32;
        uint32_t len : 32;
    } ph;
};

pcap::pcap(string filename, string name, uint32_t id) {
    pcap_filename = filename;
    parent_name = name;
    parent_id = id;
    msg_prefix = parent_name + ":" + to_string(parent_id);

    if (file_exists(pcap_filename)) {
        if (remove(filename.c_str()) != 0) {
            CEA_MSG("PCAP file " << filename <<
                " already exists and cannot be deleted. Aborting...");
            abort();
        }
    } else {
        fh = {0xa1b2c3d4, 2, 4, 0, 0, 4194304, 1};
        pcapfile.open(pcap_filename, ofstream::app);
        pcapfile.write((char*)&fh, sizeof(pcap_file_hdr));
        CEA_MSG("PCAP file created: " << pcap_filename);
        pcapfile.flush();
    }
}

pcap::~pcap() {
    pcapfile.flush();
    pcapfile.close();
}

// true if the file exists, else false
bool pcap::file_exists(const string &filename) {
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1) {
        return true;
    }
    return false;
}

void pcap::write(unsigned char *buf, uint32_t len) {
    ph.caplen = len;
    ph.len = len;
    pcapfile.write((const char*)&ph, sizeof(pcap_pkt_hdr));
    pcapfile.write((const char*)buf, len);
    pcapfile.flush();
}

// find_if with lambda predicate
cea_field_mutation_spec get_field(vector<cea_field_mutation_spec> tbl, cea_field_id id) {
    auto lit = find_if(tbl.begin(), tbl.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    if (lit==tbl.end()) {
        CEA_ERR_MSG("Internal: Unrecognized field identifier: " << id);
        abort();
    }
    return (*lit);
}

// STREAMH
//-------------
// Stream Core
//-------------

class cea_stream::core {
public:
    // ctor
    core(string name);

    // dtor
    ~core();

    // Quickly set a fixed value to a property
    void set(cea_field_id id, uint64_t value);

    // Define a complete spec for the generation of a property
    void set(cea_field_id id, cea_field_genspec spec);

    // enable or disable a stream feature
    void set(cea_stream_feature_id feature, bool mode);

    // Based on user specification of the frame, build a vector of 
    // field ids in the sequence required by the specification
    void collate_frame_fields();

    // The addition of mpls/vlan/llc/snap affects the position of ethertype and
    // length fields. update/insert type or len after arranging all fields in
    // the required sequence
    // TODO check if this is required
    void update_ethertype_and_len();

    // evaluate field length and calculate offset of all fields of this stream
    void build_field_offsets();

    // build cseq by parsing fseq and adding only mutable fields
    void filter_mutable_fields();

    // concatenate all fields reuired by the frame spec
    uint32_t splice_frame_fields(unsigned char *buf);

    // build size and payload pattern arrays
    void build_payload_arrays();

    void build_runtime_mark0();
    void build_runtime();

    void build_principal_frame();

    // process the headers and fields and prepare for generation
    void bootstrap_stream();

    // begin generation
    void mutate();

    // print the frame structure, mutable fields and stream properties
    void print_stream();

    // print a vector of field specs
    void print_fields(vector<cea_field_mutation_spec> field_group);

    // Factory reset of the stream core
    void reset();

    // Store the header pointers added to the stream
    // for generation
    vector<cea_header*> frame_headers;
    
    // frame_fields will be used to store all the fields added
    // by user by the way of adding headers
    vector<cea_field_mutation_spec> frame_fields;

    // store stream properties
    vector<cea_field_mutation_spec> stream_properties;
    
    // initialize stream to its default values
    void init_stream_properties();

    // store user defined fields
    vector<cea_field_spec> udfs;

    // mutable_fields will be used to store only those fields that will used during
    // stream generation
    vector<cea_field_mutation_spec> mutable_fields;

    // at the end of the mutation logic, the mutables will be empty
    // so if the user press start button again, the mutables will be empty
    // so a bkp version is maintained to restore the content of mutables
    // TODO check if this backup is required because we can simply call
    //      filter_mutable_fields() to regenerate the mutable fields
    vector<cea_field_mutation_spec> mutable_fields_clone;

    // functions used during mutation of strings
    void convert_string_to_uca(string address, unsigned char *op);
    void convert_mac_to_uca(string address, unsigned char *op);
    void convert_ipv4_to_uca(string address, unsigned char *op);
    void convert_ipv6_to_uca(string address, unsigned char *op);
    unsigned char convert_char_to_int(string hexNumber);
    int convert_nibble_to_int(char digit);

    string convert_int_to_ipv4(uint64_t ipAddress);
    uint64_t convert_string_ipv4_internal(string addr);

    // pcap handle for recording
    pcap *txpcap;
    pcap *rxpcap;

    // Prefixture to stream messages
    string stream_name;
    uint32_t stream_id;
    string msg_prefix;

    uint32_t hdr_len; // TODO check and rename to match intent
    uint32_t nof_sizes;
    uint32_t hdr_size;
    uint32_t meta_size;
    uint32_t crc_len;
    vector<uint32_t> vof_frame_sizes;
    vector<uint32_t> vof_computed_frame_sizes;
    vector<uint32_t> vof_payload_sizes;
    unsigned char *arof_payload_data;
    unsigned char *arof_rnd_payload_data[CEA_MAX_RND_ARRAYS];

    // TODO what are these
    unsigned char *payload_pattern;
    uint32_t payload_pattern_size;

    // principal frame
    unsigned char *pf;
};

// FIELDH
//-------------
// Field Core
//-------------

class cea_field::core {
public:
    // ctor
    core(cea_field_id id);

    // dtor
    ~core();

    // Quickly set a fixed value to a field
    void set(uint64_t id);

    // Define a complete spec for the generation of a field
    void set(cea_field_genspec spec);

    // The protocol field type that this class represents
    cea_field_id field_id;

    // A table of field structs that corresponds to the field identifiers
    // required by this field
    cea_field_mutation_spec field;

    // prefixture to field messages
    string field_name;
    string msg_prefix;
};

// UDFH
//-------------
// Udf Core
//-------------

class cea_udf::core {
public:
    // ctor
    core();

    // dtor
    ~core();

    // Define a complete spec for the generation of a field
    void set(cea_field_genspec spec);

    // A table of field structs that corresponds to the field identifiers
    // required by this field
    cea_field_mutation_spec field;

    // prefixture to field messages
    string field_name;
    string msg_prefix;
};

// HEADERH
//-------------
// Header Core
//-------------

class cea_header::core {
public:
    // ctor
    core(cea_header_type hdr_type);

    // dtor
    ~core();

    // Quickly set a fixed value to a field
    void set(cea_field_id id, uint64_t value);

    // Quickly set a fixed pattern to a field (limited set)
    void set(cea_field_id id, string value);

    // Define a complete spec for the generation of a field
    void set(cea_field_id id, cea_field_genspec spec);

    // copy the un-modified field structs from fdb corresponding to the field
    // identifiers that are required by this header
    void build_header_fields();

    void reset();

    // The protocol header type that this class represents
    cea_header_type header_type;

    // A list of field identifiers that is required by this header 
    vector<cea_field_id> field_ids_of_header;

    // A table of field structs that corresponds to the field identifiers
    // required by this header
    vector<cea_field_mutation_spec> header_fields;

    // prefixture to header messages
    string header_name;
    string msg_prefix;
};

// PORTH
//-----------
// Port Core
//-----------

class cea_port::core {
public:
    // ctor
    core(string name);

    // dtor
    ~core();

    // add stream to port queue
    void add_stream(cea_stream *stream);

    // add a command (in the form of cea_stream) to port queue
    void add_cmd(cea_stream *stream);

    // execute a command immediately does not add to port queue
    void exec_cmd(cea_stream *stream);

    // set default values
    void reset();

    // set when the port object is created
    string port_name;

    // automatically assigned when the port object is created
    // the value of the field is set from the global variable port_id
    uint32_t port_id;

    // user's test streams will be pushed into this queue (container1)
    vector<cea_stream*> streamq;

    // handle to the stream being processed
    cea_stream *current_stream;

    // prefixture to all msgs from this port
    string msg_prefix;

    // main port thread
    thread worker_tid;
    void worker();
    void start_worker();

    // execution control
    void start();
    void stop();
    void pause();
};

// TBH
//----------------
// Testbench Core
//----------------

class cea_testbench::core {
public:
    core();
    ~core();
    void add_port(cea_port *port);
    void add_stream(cea_stream *stream, cea_port *port=NULL);
    void add_cmd(cea_stream *stream, cea_port *port=NULL);
    void exec_cmd(cea_stream *stream, cea_port *port=NULL);
    void start(cea_port *port = NULL);
    void stop(cea_port *port = NULL);
    void pause(cea_port *port = NULL);
    vector<cea_port*> ports;
    string msg_prefix;
};

// HEADERI
//------------------------------------------------------------------------------
// Header implementation
//------------------------------------------------------------------------------

cea_header::cea_header(cea_header_type hdr_type) {
    impl = make_unique<core>(hdr_type); 
}

cea_header::~cea_header() = default;

void cea_header::set(cea_field_id id, uint64_t value) {
    impl->set(id, value);
}

void cea_header::set(cea_field_id id, cea_field_genspec spec) {
    impl->set(id, spec);
}

void cea_header::set(cea_field_id id, string value) {
    impl->set(id, value);
}

cea_header::core::core(cea_header_type hdr_type) {
    header_name = string("Header") + ":" + cea_header_name[hdr_type];
    msg_prefix = header_name;
    header_type = hdr_type;
    build_header_fields();
}

cea_header::core::~core() = default;

void cea_header::core::set(cea_field_id id, string value) {
    auto field = find_if(header_fields.begin(), header_fields.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    // abort if field type is not a pattern
    if (field->defaults.type == Integer) {
        CEA_ERR_MSG("The field "
        << cea_trim(mtable[id].defaults.name) << " accepts only integer values");
        abort();
    }

    // validate input
    switch(field->defaults.type) {
        case Pattern_MAC: {
            if(!regex_match(value, regex_mac)) {
                CEA_ERR_MSG("The value " << value << 
                " does not match the acceptable pattern for " 
                << cea_trim(mtable[id].defaults.name));
                abort();
            }
            break;
        } 
        case Pattern_IPv4: {
            if(!regex_match(value, regex_ipv4)) {
                CEA_ERR_MSG("The value " << value << 
                " does not match the acceptable pattern for " 
                << cea_trim(mtable[id].defaults.name));
                abort();
            }
            break;
        } 
        case Pattern_IPv6: {
            if(!regex_match(value, regex_ipv6)) {
                CEA_ERR_MSG("The value " << value << 
                " does not match the acceptable pattern for " 
                << cea_trim(mtable[id].defaults.name));
                abort();
            }
            break;
        } 
        default:{
            CEA_ERR_MSG("Input validation error for the field: "
            << cea_trim(mtable[id].defaults.name));
            abort();
        }
    }
    if (field != header_fields.end()) {
        field->gspec.gen_type = Fixed_Pattern;
        field->gspec.pattern = value;
        field->mdata.is_mutable = true;
    } else {
        CEA_ERR_MSG("The field "
        << cea_trim(mtable[id].defaults.name) << " does not belong to the "
        << cea_header_name[header_type] << " header");
        abort();
    }
}

void cea_header::core::set(cea_field_id id, uint64_t value) {
    auto field = find_if(header_fields.begin(), header_fields.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    // abort if field type is not a integer
    if (field->defaults.type != Integer) {
        CEA_ERR_MSG("The field "
        << cea_trim(mtable[id].defaults.name) << " accepts only string patterns");
        abort();
    }
    if (field != header_fields.end()) {
        field->gspec.gen_type = Fixed_Value;
        field->gspec.value = value;
        field->mdata.is_mutable = true;
    } else {
        CEA_ERR_MSG("The field "
        << cea_trim(mtable[id].defaults.name) << " does not belong to the "
        << cea_header_name[header_type] << " header");
        abort();
    }
}

void cea_header::core::set(cea_field_id id, cea_field_genspec spec) {
    auto field = find_if(header_fields.begin(), header_fields.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    if (field != header_fields.end()) {
        field->gspec = spec;
        field->mdata.is_mutable = true;
    } else {
        CEA_ERR_MSG("The field "
        << cea_trim(mtable[id].defaults.name) << " does not belong to the "
        << cea_header_name[header_type] << " header");
        abort();
    }
}

void cea_header::core::build_header_fields() {
    field_ids_of_header.clear();
    header_fields.clear();

    // extract the list of field ids that make up this header
    field_ids_of_header = header_to_field_map[header_type];

    for (auto id : field_ids_of_header) {
        auto item = get_field(mtable, id);
        header_fields.push_back(item);
    }
}

// TODO
void cea_header::core::reset() {
}

// FIELDI
//------------------------------------------------------------------------------
// Field implementation
//------------------------------------------------------------------------------

cea_field::cea_field(cea_field_id id) {
    impl = make_unique<core>(id); 
}

cea_field::~cea_field() = default;

cea_field::core::core(cea_field_id id) {
    field_id = id;
    field = get_field(mtable, field_id);
}

cea_field::core::~core() = default;

void cea_field::set(uint64_t value) {
    impl->set(value);
}

void cea_field::set(cea_field_genspec spec) {
    impl->set(spec);
}

void cea_field::core::set(uint64_t value) {
    field.gspec.value = value;
    field.gspec.gen_type = Fixed_Value;
}

void cea_field::core::set(cea_field_genspec spec) {
    field.gspec.gen_type     = spec.gen_type;
    field.gspec.value        = spec.value;
    field.gspec.pattern      = spec.pattern;
    field.gspec.step         = spec.step;
    field.gspec.min          = spec.min;
    field.gspec.max          = spec.max;
    field.gspec.count        = spec.count;
    field.gspec.repeat       = spec.repeat;
    field.gspec.mask         = spec.mask;
    field.gspec.seed         = spec.seed;
    field.gspec.start        = spec.start;
    field.gspec.make_error   = spec.make_error;
    field.gspec.value_list   = spec.value_list;
    field.gspec.pattern_list = spec.pattern_list;
}

// STREAMI
//------------------------------------------------------------------------------
// Stream implementation
//------------------------------------------------------------------------------

cea_stream::cea_stream(string name) {
    impl = make_unique<core>(name); 
}

cea_stream::~cea_stream() = default;

void cea_stream::set(cea_field_id id, uint64_t value) {
    impl->set(id, value);
}

void cea_stream::set(cea_field_id id, cea_field_genspec spec) {
    impl->set(id, spec);
}

void cea_stream::set(cea_stream_feature_id feature, bool mode) {
    impl->set(feature, mode);
}

void cea_stream::add_header(cea_header *header) {
    header->impl->msg_prefix = impl->msg_prefix + "|" + header->impl->msg_prefix;
    impl->frame_headers.push_back(header);
}

// TODO
void cea_stream::add_udf(cea_field *fld) {
// remember that UDF always overlays on the frame
// it does not insert new fields in the headerr or frames    
}

cea_stream::core::core(string name) {
    stream_name = name;
    stream_id = cea::stream_id;
    msg_prefix = stream_name + ":" + to_string(stream_id);
    cea::stream_id++;
    reset();
}

cea_stream::core::~core() = default;

void cea_stream::core::set(cea_field_id id, uint64_t value) {
    if (id == PAYLOAD_Pattern) {
        CEA_ERR_MSG("The field "
        << cea_trim(mtable[id].defaults.name) << " does not accepts integer values");
        abort();
    }
    // check if id is a property and then add to properties
    auto prop = find_if(stream_properties.begin(), stream_properties.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    if (prop != stream_properties.end()) {
        prop->gspec.gen_type = Fixed_Value; // TODO Check
        prop->gspec.value = value;
        prop->mdata.is_mutable = false;
    } else {
        CEA_ERR_MSG("The ID " << id << " does not belong to stream properties");
        abort();
    }
}

void cea_stream::core::set(cea_field_id id, cea_field_genspec spec) {
    // check if id is a property and then add to properties
    auto prop = find_if(stream_properties.begin(), stream_properties.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    if (prop != stream_properties.end()) {
        prop->gspec = spec;
        // TODO 
        // properties are different from mutable fields
        // check if is_mutation is applicable and needs to be set here
        // who will do the property mutation?
        //      mutate function
        //      what about runtime of the properties
        if (prop->gspec.gen_type != Fixed_Value || prop->gspec.gen_type != Fixed_Pattern) {
            prop->mdata.is_mutable = true;
        } else {
            prop->mdata.is_mutable = false;
        }
    } else {
        CEA_ERR_MSG("The ID " << id << " does not belong to stream properties");
        abort();
    }
}

void cea_stream::core::set(cea_stream_feature_id feature, bool mode) {
    switch (feature) {
        case PCAP_Record_Tx_Enable: {
            CEA_MSG("PCAP capture enabled @ Transmit side");
            string pcapfname = stream_name + "_tx" + + ".pcap";
            txpcap = new pcap(pcapfname, stream_name, stream_id);
            break;
            }
        case PCAP_Record_Rx_Enable: {
            CEA_MSG("PCAP capture enabled @ Receive side");
            string pcapfname = stream_name + "_rx" + + ".pcap";
            rxpcap = new pcap(pcapfname, stream_name, stream_id);
            break;
            }
        default:{
            }
    }
}

// TODO Pending verification
uint32_t cea_stream::core::splice_frame_fields(unsigned char *buf) {
    uint32_t offset = 0;
    uint64_t mrg_data = 0;
    uint64_t mrg_len = 0;
    uint64_t mrg_cnt_total = 0;
    uint64_t mrg_cntr = 0;
    bool mrg_start = false;

    for (auto f : frame_fields) {
        if(f.defaults.merge==0) {
            if (f.defaults.type == Integer) {
                cea_memcpy_ntw_byte_order(buf+offset, (char*)&f.defaults.def_value, f.defaults.len/8);
            }
            else {
                memcpy(buf+offset, f.defaults.def_pattern.data(), f.defaults.len/8);
                if (f.defaults.type == Pattern_IPv4) {
                }
            }
            offset += f.defaults.len/8;
        } else {
            if (!mrg_start) {
                mrg_start = true;
                mrg_cnt_total = f.defaults.merge + 1;
            }
            mrg_data = (mrg_data << f.defaults.len) | f.defaults.def_value;
            mrg_len = mrg_len + f.defaults.len;
            mrg_cntr++;

            if (mrg_cntr == mrg_cnt_total) {
                cea_memcpy_ntw_byte_order(buf+offset, (char*)&mrg_data, mrg_len/8);
                offset += mrg_len/8;
                mrg_data = 0; 
                mrg_len = 0;
                mrg_cntr = 0;
                mrg_cnt_total = 0;
                mrg_start = false;
            }
        }
    }
    return offset;
}
 
// TODO handle PROPERTIES mutation and runtime 
void cea_stream::core::bootstrap_stream() {
    collate_frame_fields();
    update_ethertype_and_len();
    build_field_offsets();
    filter_mutable_fields();
    build_runtime();
    print_stream();
    build_payload_arrays();
    build_principal_frame();
}

void cea_stream::core::collate_frame_fields() {
    frame_fields.clear();
    for (auto f : frame_headers) {
        frame_fields.insert(
            frame_fields.end(),
            f->impl->header_fields.begin(),
            f->impl->header_fields.end()
        );
    }
}
 
// TODO pending implementation
void cea_stream::core::update_ethertype_and_len() {
}

void cea_stream::core::build_field_offsets() {
    vector<cea_field_mutation_spec>::iterator it;
    for(it=frame_fields.begin(); it<frame_fields.end(); it++) {
        it->mdata.offset = prev(it)->defaults.len + prev(it)->mdata.offset;
        hdr_len = hdr_len + it->defaults.len;
    }
}

void cea_stream::core::filter_mutable_fields() {
    mutable_fields.clear();
    for (auto f : frame_fields) {
        if (f.mdata.is_mutable) {
            mutable_fields.push_back(f);
        }
    }
}
 
void cea_stream::core::print_fields(vector<cea_field_mutation_spec> field_group) {
    stringstream ss;
    ss.setf(ios_base::left);

    for(auto item : field_group) {
        ss << setw(5)  << left << item.defaults.id ;
        ss << setw(25) << left << item.defaults.name;
        ss << setw(25) << left << item.defaults.len;
        ss << setw(25) << left << cea_field_type_name[item.defaults.type];;
        if (item.defaults.type == Integer)
            ss << setw(25) << left << item.gspec.value;
        else
            ss << setw(25) << left << item.gspec.pattern;
        ss << setw(25) << left << cea_gen_type_name[item.gspec.gen_type];
        ss << endl;
    }
    ss << endl;
    cealog << ss.str();
}

// TODO display in a better format with sub headings
void cea_stream::core::print_stream() {
    for (auto f : frame_headers) {
        cealog << cea_header_name[f->impl->header_type] << endl;
        for (auto item : f->impl->header_fields) {
            cealog << "  |--" << item.defaults.name << endl;
        }
    }
    print_fields(stream_properties);
    print_fields(mutable_fields);
}

void cea_stream::core::build_payload_arrays() {
// TODO Support API to quickly set payload without a spec
// void set(cea_field_id id, enum of PREDFINED_PAYLOAD_PATTERN);
// void set(cea_field_id id, string pattern, bool fill=true);

    //-------------
    // size arrays
    //-------------
    auto len_item = get_field(stream_properties, FRAME_Len);
    cea_field_genspec spec = len_item.gspec;

    nof_sizes = 0;

    switch (spec.gen_type) {
        case Fixed_Value: {
            nof_sizes = 1;
            vof_frame_sizes.resize(nof_sizes);
            vof_computed_frame_sizes.resize(nof_sizes);
            vof_payload_sizes.resize(nof_sizes);
            vof_frame_sizes[0] = spec.value;
            vof_computed_frame_sizes[0] = vof_frame_sizes[0] + meta_size;
            vof_payload_sizes[0] = vof_frame_sizes[0] - (hdr_size - meta_size) - crc_len;
            break;
            }
        case Increment: {
            nof_sizes = ((spec.max - spec.min)/spec.step)+1;
            vof_frame_sizes.resize(nof_sizes);
            vof_computed_frame_sizes.resize(nof_sizes);
            vof_payload_sizes.resize(nof_sizes);
            uint32_t szidx=0;
            for (uint32_t i=spec.min; i<=spec.max; i=i+spec.step) {
                vof_frame_sizes[szidx] = i;
                vof_computed_frame_sizes[szidx] = vof_frame_sizes[szidx] + meta_size;
                vof_payload_sizes[szidx] = vof_frame_sizes[szidx] - (hdr_size - meta_size) - crc_len;
                szidx++;
            }
            break;
            }
        case Decrement: {
            nof_sizes = ((spec.min - spec.max)/spec.step)+1;
            vof_frame_sizes.resize(nof_sizes);
            vof_computed_frame_sizes.resize(nof_sizes);
            vof_payload_sizes.resize(nof_sizes);
            uint32_t szidx=0;
            for (uint32_t i=spec.min; i>=spec.max; i=i-spec.step) {
                vof_frame_sizes[szidx] = i;
                vof_computed_frame_sizes[szidx] = vof_frame_sizes[szidx] + meta_size;
                vof_payload_sizes[szidx] = vof_frame_sizes[szidx] - (hdr_size - meta_size) - crc_len;
                szidx++;
            }
            break;
            }
        case Random: {
            nof_sizes = (spec.max - spec.min) + 1;
            vof_frame_sizes.resize(nof_sizes);
            vof_computed_frame_sizes.resize(nof_sizes);
            vof_payload_sizes.resize(nof_sizes);
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> distr(spec.max, spec.min);
            uint32_t szidx=0;
            for (uint32_t szidx=spec.min; szidx>spec.max; szidx++) {
                vof_frame_sizes[szidx] = distr(gen);
                vof_computed_frame_sizes[szidx] = vof_frame_sizes[szidx] + meta_size;
                vof_payload_sizes[szidx] = vof_frame_sizes[szidx] - (hdr_size - meta_size) - crc_len;
            }
            break;
            }
        default:{
            CEA_MSG("Invalid Generation type Specified for Frame Length");
            exit(1);
            }
    }
    
    //---------------
    // payload array
    //---------------
    arof_payload_data = new unsigned char[CEA_MAX_FRAME_SIZE];
    auto pl_item = get_field(stream_properties, PAYLOAD_Pattern);
    cea_field_genspec plspec = pl_item.gspec;

    switch (plspec.gen_type) {
        case Random : {
            // create random arrays and fill it with random data
            srand(time(NULL));
            for (uint32_t idx=0; idx<CEA_MAX_RND_ARRAYS; idx++) {
                uint32_t array_size = CEA_MAX_FRAME_SIZE + CEA_RND_ARRAY_SIZE;
                arof_rnd_payload_data[idx] = new unsigned char[array_size];
                for(uint32_t offset=0; offset<array_size; offset++) {
                    int num = rand()%255;
                    memcpy(arof_rnd_payload_data[idx]+offset, (unsigned char*)&num, 1);
                }
            }
            break;
            }
        case Fixed_Pattern: {
            payload_pattern_size = plspec.pattern.size() / 2;
            payload_pattern = new unsigned char[payload_pattern_size];
            convert_string_to_uca(plspec.pattern, payload_pattern);

            uint32_t quotient = CEA_MAX_FRAME_SIZE/payload_pattern_size; 
            uint32_t remainder = CEA_MAX_FRAME_SIZE%payload_pattern_size;
            uint32_t offset = 0;

            if (plspec.repeat) {
                for (uint32_t cnt=0; cnt<quotient; cnt++) {
                    memcpy(arof_payload_data+offset, payload_pattern, payload_pattern_size);
                    offset += payload_pattern_size;
                }
                memcpy(arof_payload_data+offset, payload_pattern, remainder);
            } else {
                memcpy(arof_payload_data+offset, payload_pattern, payload_pattern_size);
            }
            delete [] payload_pattern;
            break;
            }
        case Increment_Byte: {
            uint32_t offset = 0;
            for (uint32_t idx=0; idx<CEA_MAX_FRAME_SIZE/256; idx++) {
                for (uint16_t val=0; val<256; val++) {
                    memcpy(arof_payload_data+offset, (char*)&val, 1);
                    offset++;
                }
            }
            break;
            }
        case Increment_Word: {
            uint32_t offset = 0;
            for (uint32_t idx=0; idx<CEA_MAX_FRAME_SIZE/2; idx++) {
                cea_memcpy_ntw_byte_order(arof_payload_data+offset, (char*)&idx, 2);
                offset += 2;
            }
            break;
            }
        case Decrement_Byte: {
            uint32_t offset = 0;
            for (uint32_t idx=0; idx<CEA_MAX_FRAME_SIZE/256; idx++) {
                for (int16_t val=255; val>=0; val--) {
                    memcpy(arof_payload_data+offset, (char*)&val, 1);
                    offset++;
                }
            }
            break;
            }
        case Decrement_Word: {
            uint32_t offset = 0;
            for (uint32_t val=0xFFFF; val>=0; val--) {
                memcpy(arof_payload_data+offset, (char*)&val, 2);
                offset += 2;
                if (offset > CEA_MAX_FRAME_SIZE) break;
            }
            break;
            }
        default:{
            CEA_MSG("Invalid Generation type Specified for Frame payload: " << cea_gen_type_name[plspec.gen_type]);
            exit(1);
            }
    }
}

//TODO preamble and ipv6 support is pending
void cea_stream::core::build_runtime() {
    for (auto &m : mutable_fields) {
        switch (m.defaults.type) {
            case Integer: {
                switch (m.gspec.gen_type) {
                    case Fixed_Value: {
                        m.rt.value = m.gspec.value;
                        break;
                        }
                    case Increment: {
                        m.rt.value = m.gspec.start;
                        break;
                        }
                    case Decrement: {
                        m.rt.value = m.gspec.start;
                        break;
                        }
                    case Value_List: {
                        m.rt.patterns = m.gspec.value_list;
                        break;
                        }
                    case Random: {
                        // TODO
                        break;
                        }
                    default: {}
                } // switch
                break;
                }
            case Pattern_MAC: {
                switch (m.gspec.gen_type) {
                    case Fixed_Pattern: {
                        string tmp_mac_string = m.gspec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Increment: {
                        string tmp_mac_string = m.gspec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Decrement: {
                        string tmp_mac_string = m.gspec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Pattern_List: {
                        m.rt.patterns.resize(0);
                        for (auto val : m.gspec.pattern_list) {
                            val.erase(remove(val.begin(), val.end(), ':'), val.end());
                            uint64_t tmp_mac = stol(val, 0, 16);
                            m.rt.patterns.push_back(tmp_mac);
                        }
                        break;
                        }
                    default: {}
                } // switch
                break;
                }
            case Pattern_IPv4: {
                switch (m.gspec.gen_type) {
                    case Fixed_Pattern: {
                        string tmp_mac_string = m.gspec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), '.'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Increment: {
                        string tmp_mac_string = m.gspec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), '.'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Decrement: {
                        string tmp_mac_string = m.gspec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), '.'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Pattern_List: {
                        m.rt.patterns.resize(0);
                        for (auto val : m.gspec.pattern_list) {
                            val.erase(remove(val.begin(), val.end(), '.'), val.end());
                            uint64_t tmp_mac = stol(val, 0, 16);
                            m.rt.patterns.push_back(tmp_mac);
                        }
                        break;
                        }
                    default: {
                        // ignore
                        }
                } // switch
                break;
                }
            default: {}
        }
    } // for
}

// // TODO
// // assign default value to runtime
// // TODO CRITICAL
// // when assigning random spec to fields. the test first creates a empty spec,
// // sets random and assigns to the field. In this process the default value is
// // erased when assigning the empty spec+random to the field
// // so when build_runtime is called the default values/patterns are void and
// // hence results in error when trying to access default and convert default
// // values for patterns
// // TODO POSSIBLE SOLUTION: To be decided
// //      -> call build_runtime after the principal frame is generated ??
// void cea_stream::core::build_runtime_mark0() {
// //    for (auto &f : frame_fields) {
// //        if (f.type == Integer) {
// //            f.rt.gen_value = f.spec.value;
// //        } else {
// //            switch (f.type) {
// //                case Pattern_PRE:{
// //                    convert_string_to_uca(f.spec.pattern, f.rt.patterns);
// //                    break;
// //                    }
// //                case Pattern_MAC:{
// //                    convert_mac_to_uca(f.spec.pattern, f.rt.patterns);
// //                    break;
// //                    }
// //                case Pattern_IPv4:{
// //                    convert_ipv4_to_uca(f.spec.pattern, f.rt.patterns);
// //                    break;
// //                    }
// //                case Pattern_IPv6:{
// //                    convert_ipv6_to_uca(f.spec.pattern, f.rt.patterns);
// //                    break;
// //                    }
// //                default:{
// //                    CEA_ERR_MSG("Invalid Generation type Specified"); // TODO
// //                    break;
// //                }
// //            }    
// //        }
// //    }
// }

// TODO Incomplete implementation
void cea_stream::core::build_principal_frame() {

    // print_fields(frame_fields);
    splice_frame_fields(pf);

    auto len_item = get_field(stream_properties, FRAME_Len);
    cea_field_genspec lenspec = len_item.gspec;

    auto pl_item = get_field(stream_properties, PAYLOAD_Pattern);
    cea_field_genspec plspec = pl_item.gspec;

    uint32_t ploffset = hdr_len/8;

    if (plspec.gen_type == Random)
        memcpy(pf+ploffset, arof_rnd_payload_data[0], lenspec.value);
    else 
        memcpy(pf+ploffset, arof_payload_data, lenspec.value);

    print_uchar_array(pf, ploffset+lenspec.value);
}


// TODO nof frames in outer loop    
// TODO what if there are no mutables
// TODO enclose mutate with perf timers
void cea_stream::core::mutate() {
    for (auto m=begin(mutable_fields); m!=end(mutable_fields); m++) {
        switch(m->defaults.type) {
            case Integer: {
                switch(m->gspec.gen_type) {
                    case Fixed_Value: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset, (char*)&m->gspec.value, m->defaults.len/8);
                        m->mdata.is_mutable = false;
                        mutable_fields.erase(m); m++;
                        break;
                        }
                    case Value_List: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset, (char*)&m->rt.patterns[m->rt.idx], m->defaults.len/8);
                        if (m->rt.idx == m->rt.patterns.size()-1) {
                            if (m->gspec.repeat) {
                                m->rt.idx = 0;
                            } else {
                                mutable_fields.erase(m); m++;
                            }
                        } else {
                            m->rt.idx++;
                        }
                        break;
                        }
                    case Increment: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset, (char*)&m->rt.value, m->defaults.len/8);
                        if (m->rt.count == m->gspec.count) {
                            if (m->gspec.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->gspec.start;
                            } else {
                                mutable_fields.erase(m); m++;
                            }
                        } else {
                            // TODO check overflow
                            m->rt.value += m->gspec.step;
                            m->rt.count++;
                        }
                        break;
                        }
                    case Decrement: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset, (char*)&m->rt.value, m->defaults.len/8);
                        if (m->rt.count == m->gspec.count) {
                            if (m->gspec.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->gspec.start;
                            } else {
                                mutable_fields.erase(m); m++;
                            }
                        } else {
                            // TODO check underflow
                            m->rt.value -= m->gspec.step;
                            m->rt.count++;
                        }
                        break;
                        }
                    case Random: {
                        // TODO after research
                        break;
                        }
                    default: {}
                }
                break;
                } // Integer
            case Pattern_MAC:
            case Pattern_IPv4: {
                switch(m->gspec.gen_type) {
                    case Fixed_Pattern: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset, (char*)&m->rt.value, m->defaults.len/8);
                        m->mdata.is_mutable = false;
                        mutable_fields.erase(m); // m++; // TODO iterator increment
                                           // fails if it is done after the last
                                           // element is deleted, swap erase and
                                           // increment
                        break;
                        }
                    case Pattern_List: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset, (char*)&m->rt.patterns[m->rt.idx], m->defaults.len/8);
                        if (m->rt.idx == m->rt.patterns.size()-1) {
                            if (m->gspec.repeat) {
                                m->rt.idx = 0;
                            } else {
                                mutable_fields.erase(m); // m++;
                            }
                        } else {
                            m->rt.idx++;
                        }
                        break;
                        }
                    case Increment: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset, (char*)&m->rt.value, m->defaults.len/8);
                        if (m->rt.count == m->gspec.count) {
                            if (m->gspec.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->gspec.start;
                            } else {
                                mutable_fields.erase(m); // m++;
                            }
                        } else {
                            // TODO check overflow
                            m->rt.value += m->gspec.step;
                            m->rt.count++;
                        }
                        break;
                        }
                    case Decrement: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset, (char*)&m->rt.value, m->defaults.len/8);
                        if (m->rt.count == m->gspec.count) {
                            if (m->gspec.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->gspec.start;
                            } else {
                                mutable_fields.erase(m); // m++;
                            }
                        } else {
                            // TODO check underflow
                            m->rt.value -= m->gspec.step;
                            m->rt.count++;
                        }
                        break;
                        }
                    case Random: {
                        // TODO after research
                        break;
                        }
                    default: {}
                }
                break;
                }
            default: {}
        }
    }
// TODO copy frame to transmit buffer    
}
 
void cea_stream::core::init_stream_properties() {
    stream_properties.clear();
    vector<cea_field_id> prop_ids =  header_to_field_map[PROPERTIES];

    for (auto id : prop_ids) {
        auto item = get_field(mtable, id);
        stream_properties.push_back(item);
    }
}
 
void cea_stream::core::reset() {
    frame_headers.clear();

    // add metadata to headers by default
    // cea_header *meta = new cea_header(META);
    // frame_headers.push_back(meta);

    frame_fields.clear();
    udfs.clear();
    init_stream_properties();

    // TODO memory leak when reset is done twice in same test
    pf = new unsigned char [CEA_PF_SIZE];

    payload_pattern_size = 0;

    // TODO why does the following crash
    // if (payload_pattern != nullptr) {
    //     delete [] payload_pattern;
    //     payload_pattern = nullptr;
    // }

    hdr_len = 0;
}

void cea_stream::core::convert_string_to_uca(string address, unsigned char *op) {
    vector <string> tokens;

    for (uint32_t i=0; i<address.size(); i+=2) {
        tokens.push_back(address.substr(i, 2));
    }
    for (uint32_t i=0; i<address.size()/2; i++) {
        op[i]= convert_char_to_int(tokens[i]);
    }
}

void cea_stream::core::convert_mac_to_uca(string address, unsigned char *op) {
    stringstream check1(address);
    vector <string> tokens;
    string intermediate;

    while(getline(check1, intermediate, ':')) {
        tokens.push_back(intermediate);
    }
    for (uint32_t i=0; i<6; i++) { // TODO remove hardcoded value
        op[i]= convert_char_to_int(tokens[i]);
    }
}

void cea_stream::core::convert_ipv4_to_uca(string address, unsigned char *op) {
    stringstream check1(address);
    string intermediate;
    int i = 0;

    while(getline(check1, intermediate, '.')) {
        op[i] = stoi(intermediate);
        i++;
    }
}

void cea_stream::core::convert_ipv6_to_uca(string address, unsigned char *op) {
    string ipv6addr_tmp = address;
    vector <string> tokens;

    ipv6addr_tmp.erase(remove(ipv6addr_tmp.begin(), ipv6addr_tmp.end(), ':'),
        ipv6addr_tmp.end());

    for (size_t i=0; i<ipv6addr_tmp.size(); i+=2) {
        tokens.push_back(ipv6addr_tmp.substr(i, 2));
    }
    for (uint32_t i=0; i<16; i++) { // TODO remove hardcoded value
        op[i]= convert_char_to_int(tokens[i]);
    }
}

string cea_stream::core::convert_int_to_ipv4(uint64_t ipAddress) {
    uint32_t octet1 = (ipAddress >> 24) & 0xFF;
    uint32_t octet2 = (ipAddress >> 16) & 0xFF;
    uint32_t octet3 = (ipAddress >> 8) & 0xFF;
    uint32_t octet4 = ipAddress & 0xFF;
    return (to_string(octet1) + "." +
        to_string(octet2) + "." +
        to_string(octet3) + "." +
        to_string(octet4));
}

uint64_t cea_stream::core::convert_string_ipv4_internal(string addr) {
    stringstream s;
    string intermediate;
    stringstream check1(addr);
    int i = 0;
    while(getline(check1, intermediate, '.')) {
        s << setfill('0') << setw(2) << hex << stoi(intermediate);
        i++;
    }
    return stoul(s.str(), 0, 16);
}

int cea_stream::core::convert_nibble_to_int (char digit) {
    int asciiOffset, digitValue;
    if (digit >= 48 && digit <= 57) {
        // code for '0' through '9'
        asciiOffset = 48;
        digitValue = digit - asciiOffset;
        return digitValue;
    } else if (digit >= 65 && digit <= 70) {
        // digit is 'A' through 'F'
        asciiOffset = 55;
        digitValue = digit - asciiOffset;
        return digitValue;
    } else if (digit >= 97 && digit <= 122) {
        // code for 'a' through 'f'
        asciiOffset = 87;
        digitValue = digit - asciiOffset;
        return digitValue;
    } else {
        // TODO illegal digit
    }
    return 0;
}

unsigned char cea_stream::core::convert_char_to_int(string hexNumber) {
     unsigned char aChar;
     char highOrderDig = hexNumber[0];
     char lowOrderDig  = hexNumber[1];
     int lowOrderValue = convert_nibble_to_int(lowOrderDig);
     //  convert lowOrderDig to number from 0 to 15
     int highOrderValue = convert_nibble_to_int(highOrderDig);
     // convert highOrderDig to number from 0 to 15
     aChar = lowOrderValue + 16 * highOrderValue;
     return aChar;
}

// PORTI
//------------------------------------------------------------------------------
// Port Implementation
//------------------------------------------------------------------------------

cea_port::cea_port(string name) {
    impl = make_unique<core>(name);
}

cea_port::~cea_port() = default;

cea_port::core::core(string name) {
    port_id = cea::port_id;
    cea::port_id++;
    port_name = name + ":" + to_string(port_id);
    reset();
    CEA_MSG("Proxy created with name=" << name << " and id=" << port_id);
}

cea_port::core::~core() = default;

void cea_port::core::reset() {
    msg_prefix = port_name;
}

void cea_port::core::worker() {
    vector<cea_stream*>::iterator it;

    for (it = streamq.begin(); it != streamq.end(); it++) {
        current_stream = *it;
        current_stream->impl->bootstrap_stream();
        current_stream->impl->mutate();
    }
}

void cea_port::core::start_worker() {
    worker_tid = thread(&cea_port::core::worker, this);
    char name[16];
    sprintf(name, "worker_%d", port_id);
    pthread_setname_np(worker_tid.native_handle(), name);
}


void cea_port::add_stream(cea_stream *stream) {
    impl->add_stream(stream);
}

void cea_port::add_cmd(cea_stream *stream) {
    impl->add_cmd(stream);
}

void cea_port::exec_cmd(cea_stream *stream) {
    impl->exec_cmd(stream);
}

void cea_port::core::add_stream(cea_stream *stream) {
    streamq.push_back(stream);
}

void cea_port::core::add_cmd(cea_stream *stream) {
    add_stream(stream);
}

void cea_port::core::exec_cmd(cea_stream *stream) {
// TODO pending implementation
}

void cea_port::core::start() {
    start_worker();
}

void cea_port::core::stop() {
// TODO pending implementation
}

void cea_port::core::pause() {
// TODO pending implementation
}
 
// TBI
//------------------------------------------------------------------------------
// Testbench Implementation
//------------------------------------------------------------------------------

cea_testbench::cea_testbench() {
    impl = make_unique<core>();
}

cea_testbench::~cea_testbench() = default;

cea_testbench::core::core() {
}

cea_testbench::core::~core() = default;

void cea_testbench::add_port(cea_port *port) {
    impl->add_port(port);
}

void cea_testbench::add_stream(cea_stream *stream, cea_port *port) {
    impl->add_stream(stream, port);
}

void cea_testbench::add_cmd(cea_stream *stream, cea_port *port) {
    impl->add_cmd(stream, port);
}

void cea_testbench::exec_cmd(cea_stream *stream, cea_port *port) {
    impl->exec_cmd(stream, port);
}

void cea_testbench::core::add_port(cea_port *port) {
    ports.push_back(port);
}

void cea_testbench::core::add_stream(cea_stream *stream, cea_port *port) {
    if (ports.size() == 0) {
        CEA_ERR_MSG("Cannot add stream to port since no ports are added to the testbench");
    }
    if (port != NULL) {
        vector<cea_port*>::iterator it;
        for (it = ports.begin(); it != ports.end(); it++) {
            if ((*it)->impl->port_id == port->impl->port_id) {
                uint32_t idx = distance(ports.begin(), it);
                ports[idx]->add_stream(stream);
            }
        }
    } else {
        for (uint32_t idx=0; idx<ports.size(); idx++) {
            ports[idx]->add_stream(stream);
        }
    }
}

void cea_testbench::core::add_cmd(cea_stream *stream, cea_port *port) {
    add_stream(stream, port);
}

void cea_testbench::core::exec_cmd(cea_stream *stream, cea_port *port) {
// TODO pending implementation
}

void cea_testbench::start(cea_port *port) {
    impl->start(port);
}

void cea_testbench::stop(cea_port *port) {
    impl->stop(port);
}

void cea_testbench::pause(cea_port *port) {
    impl->pause(port);
}

void cea_testbench::core::start(cea_port *port) {
    if (port != NULL) {
        vector<cea_port*>::iterator it;

        // start threads
        for (it = ports.begin(); it != ports.end(); it++) {
            if ((*it)->impl->port_id == port->impl->port_id) {
                uint32_t idx = distance(ports.begin(), it);
                ports[idx]->impl->start();
            }
        }
        // join threads
        for (it = ports.begin(); it != ports.end(); it++) {
            if ((*it)->impl->port_id == port->impl->port_id) {
                uint32_t idx = distance(ports.begin(), it);
                ports[idx]->impl->worker_tid.join();
            }
        }
    } else {
        // start threads
        for (uint32_t idx=0; idx<ports.size(); idx++) {
            ports[idx]->impl->start();
        }
        // join threads
        for (uint32_t idx=0; idx<ports.size(); idx++) {
            ports[idx]->impl->worker_tid.join();
        }
    }
}

void cea_testbench::core::stop(cea_port *port) {
    if (port != NULL) {
        vector<cea_port*>::iterator it;
        for (it = ports.begin(); it != ports.end(); it++) {
            if ((*it)->impl->port_id == port->impl->port_id) {
                uint32_t idx = distance(ports.begin(), it);
                ports[idx]->impl->stop();
            }
        }
    } else {
        for (uint32_t idx=0; idx<ports.size(); idx++) {
            ports[idx]->impl->stop();
        }
    }
}

void cea_testbench::core::pause(cea_port *port) {
    if (port != NULL) {
        vector<cea_port*>::iterator it;
        for (it = ports.begin(); it != ports.end(); it++) {
            if ((*it)->impl->port_id == port->impl->port_id) {
                uint32_t idx = distance(ports.begin(), it);
                ports[idx]->impl->pause();
            }
        }
    } else {
        for (uint32_t idx=0; idx<ports.size(); idx++) {
            ports[idx]->impl->pause();
        }
    }
}
 
// UDFI
//------------------------------------------------------------------------------
// Udf implementation
//------------------------------------------------------------------------------

cea_udf::cea_udf() {
    impl = make_unique<core>(); 
}

cea_udf::~cea_udf() = default;

cea_udf::core::core() {
    field = {};
}

cea_udf::core::~core() = default;

void cea_udf::set(cea_field_genspec spec) {
    impl->set(spec);
}

void cea_udf::core::set(cea_field_genspec spec) {
    field.gspec.gen_type     = spec.gen_type;
    field.gspec.value        = spec.value;
    field.gspec.pattern      = spec.pattern;
    field.gspec.step         = spec.step;
    field.gspec.min          = spec.min;
    field.gspec.max          = spec.max;
    field.gspec.count        = spec.count;
    field.gspec.repeat       = spec.repeat;
    field.gspec.mask         = spec.mask;
    field.gspec.seed         = spec.seed;
    field.gspec.start        = spec.start;
    field.gspec.make_error   = spec.make_error;
    field.gspec.value_list   = spec.value_list;
    field.gspec.pattern_list = spec.pattern_list;
}

} // namespace
