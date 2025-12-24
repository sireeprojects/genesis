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

#define CEA_WEIGHTED_DISTR_RECYCLE_LIMIT 1000000

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
    bool is_protolist;
    bool is_auto;
    string name;
    uint64_t value;
    vector <unsigned char> pattern;
    cea_field_type type;
};

struct cea_field_mutation_data {
    bool is_mutable;
    uint32_t offset;
};

struct cea_field_random {
    mt19937 engine;
    uniform_int_distribution<uint64_t> ud;
    discrete_distribution<uint64_t> wd;
    vector<uint64_t> wd_lenghts;
    vector<double> wd_weights;
};
 
struct cea_field_mutation_spec {
    cea_field_spec defaults;
    cea_field_genspec gspec;
    cea_field_runtime rt;
    cea_field_mutation_data mdata;
    cea_field_random rnd;
};

typedef enum  {
    NEW_FRAME,
    TRANSMIT
} mutation_states;

vector<unsigned char>def_pre_pattern    = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x5d};
vector<unsigned char>def_dstmac_pattern = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
vector<unsigned char>def_srcmac_pattern = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
vector<unsigned char>def_srcip4_pattern = {0xc0, 0xa8, 0x00, 0x01};
vector<unsigned char>def_dstip4_pattern = {0x11, 0x22, 0x33, 0x44};
vector<unsigned char>def_srcip6_pattern = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
vector<unsigned char>def_dstip6_pattern = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

vector<cea_field_mutation_spec> mtable = {
{ /*defaults*/ {0, MAC_Preamble          , 64 , 0, 0, "MAC_Preamble          ", 0                , def_pre_pattern    , Pattern_PRE }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"55555555555555d"  , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, MAC_Dest_Addr         , 48 , 0, 0, "MAC_Dest_Addr         ", 0                , def_dstmac_pattern , Pattern_MAC }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"01:02:03:04:05:06", 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, MAC_Src_Addr          , 48 , 0, 0, "MAC_Src_Addr          ", 0                , def_srcmac_pattern , Pattern_MAC }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"0a:0b:0c:0d:0e:0f", 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, MAC_Len               , 16 , 0, 0, "MAC_Len               ", 46               , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {46               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, MAC_Ether_Type        , 16 , 0, 0, "MAC_Ether_Type        ", 0x0800           , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0x0800           , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, MAC_Fcs               , 32 , 0, 0, "MAC_Fcs               ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, LLC_Dsap              , 8  , 0, 0, "LLC_Dsap              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, LLC_Ssap              , 8  , 0, 0, "LLC_Ssap              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, LLC_Control           , 8  , 0, 0, "LLC_Control           ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, SNAP_Oui              , 24 , 0, 0, "SNAP_Oui              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, SNAP_Pid              , 16 , 0, 0, "SNAP_Pid              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, IPv4_Version          , 4  , 0, 0, "IPv4_Version          ", 4                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {4                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, IPv4_IHL              , 4  , 0, 0, "IPv4_IHL              ", 5                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {5                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv4_Tos              , 8  , 0, 0, "IPv4_Tos              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv4_Total_Len        , 16 , 0, 0, "IPv4_Total_Len        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv4_Id               , 16 , 0, 0, "IPv4_Id               ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, IPv4_Flags            , 3  , 0, 0, "IPv4_Flags            ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, IPv4_Frag_Offset      , 13 , 0, 0, "IPv4_Frag_Offset      ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv4_TTL              , 8  , 0, 0, "IPv4_TTL              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv4_Protocol         , 8  , 0, 0, "IPv4_Protocol         ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv4_Hdr_Csum         , 16 , 0, 0, "IPv4_Hdr_Csum         ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv4_Src_Addr         , 32 , 0, 0, "IPv4_Src_Addr         ", 0                , def_srcip4_pattern , Pattern_IPv4}, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"192.168.0.1"      , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv4_Dest_Addr        , 32 , 0, 0, "IPv4_Dest_Addr        ", 0                , def_dstip4_pattern , Pattern_IPv4}, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"255.255.255.255"  , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv4_Opts             , 0  , 0, 0, "IPv4_Opts             ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv4_Pad              , 0  , 0, 0, "IPv4_Pad              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {2, IPv6_Version          , 4  , 0, 0, "IPv6_Version          ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, IPv6_Traffic_Class    , 8  , 0, 0, "IPv6_Traffic_Class    ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, IPv6_Flow_Label       , 20 , 0, 0, "IPv6_Flow_Label       ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv6_Payload_Len      , 16 , 0, 0, "IPv6_Payload_Len      ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv6_Next_Hdr         , 8  , 0, 0, "IPv6_Next_Hdr         ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv6_Hop_Limit        , 8  , 0, 0, "IPv6_Hop_Limit        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv6_Src_Addr         , 128, 0, 0, "IPv6_Src_Addr         ", 0                , def_srcip6_pattern , Pattern_IPv6}, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"0.0.0.0.0.0.0.0"  , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, IPv6_Dest_Addr        , 128, 0, 0, "IPv6_Dest_Addr        ", 0                , def_dstip6_pattern , Pattern_IPv6}, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"0.0.0.0.0.0.0.0"  , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, TCP_Src_Port          , 16 , 0, 0, "TCP_Src_Port          ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, TCP_Dest_Port         , 16 , 0, 0, "TCP_Dest_Port         ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, TCP_Seq_Num           , 32 , 0, 0, "TCP_Seq_Num           ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, TCP_Ack_Num           , 32 , 0, 0, "TCP_Ack_Num           ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {7, TCP_Data_Offset       , 4  , 0, 0, "TCP_Data_Offset       ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, TCP_Reserved          , 6  , 0, 0, "TCP_Reserved          ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, TCP_Urg               , 1  , 0, 0, "TCP_Urg               ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, TCP_Ack               , 1  , 0, 0, "TCP_Ack               ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, TCP_Psh               , 1  , 0, 0, "TCP_Psh               ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, TCP_Rst               , 1  , 0, 0, "TCP_Rst               ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, TCP_Syn               , 1  , 0, 0, "TCP_Syn               ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, TCP_Fin               , 1  , 0, 0, "TCP_Fin               ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, TCP_Window            , 16 , 0, 0, "TCP_Window            ", 64               , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {64               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, TCP_Csum              , 16 , 0, 0, "TCP_Csum              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, TCP_Urg_Ptr           , 16 , 0, 0, "TCP_Urg_Ptr           ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, TCP_Opts              , 0  , 0, 0, "TCP_Opts              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, TCP_Pad               , 0  , 0, 0, "TCP_Pad               ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, UDP_Src_Port          , 16 , 0, 0, "UDP_Src_Port          ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, UDP_Dest_Port         , 16 , 0, 0, "UDP_Dest_Port         ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, UDP_Len               , 16 , 0, 0, "UDP_Len               ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, UDP_Csum              , 16 , 0, 0, "UDP_Csum              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, ARP_Hw_Type           , 16 , 0, 0, "ARP_Hw_Type           ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, ARP_Proto_Type        , 16 , 0, 0, "ARP_Proto_Type        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, ARP_Hw_Len            , 8  , 0, 0, "ARP_Hw_Len            ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, ARP_Proto_Len         , 8  , 0, 0, "ARP_Proto_Len         ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, ARP_Opcode            , 16 , 0, 0, "ARP_Opcode            ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, ARP_Sender_Hw_Addr    , 48 , 0, 0, "ARP_Sender_Hw_Addr    ", 0                , def_srcmac_pattern , Pattern_MAC }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"00:00:00:00:00:00", 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, ARP_Sender_Proto_addr , 32 , 0, 0, "ARP_Sender_Proto_addr ", 0                , def_srcip4_pattern , Pattern_IPv4}, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"0.0.0.0"          , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, ARP_Target_Hw_Addr    , 48 , 0, 0, "ARP_Target_Hw_Addr    ", 0                , def_dstmac_pattern , Pattern_MAC }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"00:00:00:00:00:00", 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, ARP_Target_Proto_Addr , 32 , 0, 0, "ARP_Target_Proto_Addr ", 0                , def_dstip4_pattern , Pattern_IPv4}, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"0.0.0.0"          , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {2, MPLS_Label            , 20 , 0, 0, "MPLS_Label            ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, MPLS_Exp              , 3  , 0, 0, "MPLS_Exp              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, MPLS_Stack            , 1  , 0, 0, "MPLS_Stack            ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, MPLS_Ttl              , 8  , 0, 0, "MPLS_Ttl              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, VLAN_Tpi              , 16 , 0, 0, "VLAN_Tpi              ", 0x8100           , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0x8100           , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {2, VLAN_Tci_Pcp          , 3  , 0, 0, "VLAN_Tci_Pcp          ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, VLAN_Tci_Cfi          , 1  , 0, 0, "VLAN_Tci_Cfi          ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {1, VLAN_Vid              , 12 , 0, 0, "VLAN_Vid              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, MAC_Control           , 16 , 0, 0, "MAC_Control           ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, MAC_Control_Opcode    , 16 , 0, 0, "MAC_Control_Opcode    ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Pause_Quanta          , 16 , 0, 0, "Pause_Quanta          ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Priority_En_Vector    , 16 , 0, 0, "Priority_En_Vector    ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Pause_Quanta_0        , 16 , 0, 0, "Pause_Quanta_0        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Pause_Quanta_1        , 16 , 0, 0, "Pause_Quanta_1        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Pause_Quanta_2        , 16 , 0, 0, "Pause_Quanta_2        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Pause_Quanta_3        , 16 , 0, 0, "Pause_Quanta_3        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Pause_Quanta_4        , 16 , 0, 0, "Pause_Quanta_4        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Pause_Quanta_5        , 16 , 0, 0, "Pause_Quanta_5        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Pause_Quanta_6        , 16 , 0, 0, "Pause_Quanta_6        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Pause_Quanta_7        , 16 , 0, 0, "Pause_Quanta_7        ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, FRAME_Len             , 32 , 0, 0, "FRAME_Len             ", 64               , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {64               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, PAYLOAD_Pattern       , 0  , 0, 0, "PAYLOAD_Pattern       ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {"00"               , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, STREAM_Traffic_Type   , 32 , 0, 0, "STREAM_Traffic_Type   ", Continuous       , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {Continuous       , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, STREAM_Burst_Size     , 32 , 0, 0, "STREAM_Burst_Size     ", 10               , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {10               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, STREAM_Traffic_Control, 32 , 0, 0, "STREAM_Traffic_Control", Stop_After_Stream, {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {Stop_After_Stream, 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, STREAM_Ipg            , 32 , 0, 0, "STREAM_Ipg            ", 12               , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {12               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, STREAM_Isg            , 32 , 0, 0, "STREAM_Ifg            ", 12               , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {12               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, STREAM_Ibg            , 32 , 0, 0, "STREAM_Ibg            ", 12               , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {12               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, STREAM_Bandwidth      , 32 , 0, 0, "STREAM_Bandwidth      ", 100              , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {100              , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, STREAM_Start_Delay    , 32 , 0, 0, "STREAM_Start_Delay    ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, UDF                   , 0  , 0, 0, "UDF                   ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, META_Len              , 32 , 0, 0, "META_Len              ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, META_Ipg              , 32 , 0, 0, "META_Ipg              ", 12               , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {12               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, META_Preamble         , 64 , 0, 0, "META_Preamble         ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, META_Pad1             , 64 , 0, 0, "META_Pad1             ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, META_Pad2             , 64 , 0, 0, "META_Pad2             ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, META_Pad3             , 64 , 0, 0, "META_Pad3             ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, META_Pad4             , 64 , 0, 0, "META_Pad4             ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, META_Pad5             , 64 , 0, 0, "META_Pad5             ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, META_Pad6             , 64 , 0, 0, "META_Pad6             ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, Zeros_8Bit            , 8  , 0, 0, "Zeros_8Bit            ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
{ /*defaults*/ {0, TCP_Total_Len         , 16 , 0, 0, "TCP_Total_Len         ", 0                , {0x00}             , Integer     }, /*gspec*/ {Fixed_Value , /*nmr*/ {0                , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {}, ""}, /*str*/ {""                 , 0, "", "", 0, 0, "", 0, "", 0, {}}}, /*rt*/ {0, {}, 0, 0}, /*mdata*/ {0, 0}, /*rnd*/ {}},
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
             STREAM_Burst_Size,
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
            MAC_Src_Addr,
            MAC_Ether_Type
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
    "Value_List",
    "Increment",
    "Decrement",
    "Random",
    "Random_In_Range",
    "Weighted_Distribution",
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

bool is_number_in_range(uint64_t number, uint64_t lower_bound, uint64_t upper_bound) {
    return (number >= lower_bound && number <= upper_bound);
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

void print_uchar_array_1n (unsigned char* tmp, int len, string hdr) {
    stringstream s;
    s.str("");
    uint32_t idx = 0;

    for (int x=0; x<len/16; x++) {
        for (int y=0; y<16; y++) {
            s << noshowbase << setw(2) << setfill('0')
              << hex << uint16_t(tmp[idx]) << " ";
            idx++;
        }
        // s << endl;
    }
    int spacer = 0;
    for (int x=idx; x<len; x++) {
       s<<noshowbase<<setw(2)<<setfill('0') <<hex<<uint16_t(tmp[idx])<<" ";
       idx++;
       spacer++;
       if (spacer == 8) s << " ";
    }
    cealog << s.str()<<endl;
    fflush (stdout);
}

void print_uchar_array (unsigned char* tmp, int len, string hdr) {
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
    cealog << hdr << " " << endl << s.str()<<endl << endl;
    // cealog << hdr << " " << string(37, '-') << endl << s.str()<<endl << string(48, '-') << endl;
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
    auto result = find_if(tbl.begin(), tbl.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    if (result == tbl.end()) {
        CEA_ERR_MSG("FATAL ERROR: " << "Unrecognized field id (" << id <<") passed to " << __func__);
        abort();
    }
    return (*result);
}


} // namespace
