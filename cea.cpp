/*
NOTES:
marco CEA_DEBUG - to include debug code and to generate debug library
THINGS TO DO:
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
//       Every workstation will have a port_id and stream_id = 0
//       Re-design in such a way that these values continue over multiple w/s
uint32_t stream_id = 0;
uint32_t port_id = 0;

// CRC32
const uint32_t crc32_tab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

enum cea_field_type {
    Integer,
    // Pattern, // TODO Required?
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
    uint64_t value;
    vector<uint64_t> patterns;
    uint32_t count;
    uint32_t idx;
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
    uint64_t def_value;
    vector <unsigned char> def_pattern;
    cea_field_type type;
    cea_gen_spec spec;
    cea_runtime rt;
};

vector<unsigned char>def_pre_pattern    = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x5d};
vector<unsigned char>def_dstmac_pattern = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
vector<unsigned char>def_srcmac_pattern = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
vector<unsigned char>def_srcip4_pattern = {0xc0, 0xa8, 0x00, 0x01};
vector<unsigned char>def_dstip4_pattern = {0xff, 0xff, 0xff, 0xff};
vector<unsigned char>def_srcip6_pattern = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
vector<unsigned char>def_dstip6_pattern = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

vector<cea_field_spec> fdb = {
{false, 0, MAC_Preamble          , 64 , 0, 0, 0, "MAC_Preamble          ", 0                , def_pre_pattern    , Pattern_PRE,  { Fixed_Pattern, 0                , "55555555555555d"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, MAC_Dest_Addr         , 48 , 0, 0, 0, "MAC_Dest_Addr         ", 0                , def_dstmac_pattern , Pattern_MAC,  { Fixed_Pattern, 0                , "01:02:03:04:05:06", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, MAC_Src_Addr          , 48 , 0, 0, 0, "MAC_Src_Addr          ", 0                , def_srcmac_pattern , Pattern_MAC,  { Fixed_Pattern, 0                , "0a:0b:0c:0d:0e:0f", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, MAC_Len               , 16 , 0, 0, 0, "MAC_Len               ", 46               , {0x00}             , Integer,      { Fixed_Value  , 46               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, MAC_Ether_Type        , 16 , 0, 0, 0, "MAC_Ether_Type        ", 0x0800           , {0x00}             , Integer,      { Fixed_Value  , 0x0800           , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, MAC_Fcs               , 32 , 0, 0, 0, "MAC_Fcs               ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, LLC_Dsap              , 8  , 0, 0, 0, "LLC_Dsap              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, LLC_Ssap              , 8  , 0, 0, 0, "LLC_Ssap              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, LLC_Control           , 8  , 0, 0, 0, "LLC_Control           ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, SNAP_Oui              , 24 , 0, 0, 0, "SNAP_Oui              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, SNAP_Pid              , 16 , 0, 0, 0, "SNAP_Pid              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, IPv4_Version          , 4  , 0, 0, 0, "IPv4_Version          ", 4                , {0x00}             , Integer,      { Fixed_Value  , 4                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, IPv4_IHL              , 4  , 0, 0, 0, "IPv4_IHL              ", 5                , {0x00}             , Integer,      { Fixed_Value  , 5                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv4_Tos              , 8  , 0, 0, 0, "IPv4_Tos              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv4_Total_Len        , 16 , 0, 0, 0, "IPv4_Total_Len        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv4_Id               , 16 , 0, 0, 0, "IPv4_Id               ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, IPv4_Flags            , 3  , 0, 0, 0, "IPv4_Flags            ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, IPv4_Frag_Offset      , 13 , 0, 0, 0, "IPv4_Frag_Offset      ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv4_TTL              , 8  , 0, 0, 0, "IPv4_TTL              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv4_Protocol         , 8  , 0, 0, 0, "IPv4_Protocol         ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv4_Hdr_Csum         , 16 , 0, 0, 0, "IPv4_Hdr_Csum         ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv4_Src_Addr         , 32 , 0, 0, 0, "IPv4_Src_Addr         ", 0                , def_srcip4_pattern , Pattern_IPv4, { Fixed_Pattern, 0                , "192.168.0.1"      , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv4_Dest_Addr        , 32 , 0, 0, 0, "IPv4_Dest_Addr        ", 0                , def_dstip4_pattern , Pattern_IPv4, { Fixed_Pattern, 0                , "255.255.255.255"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv4_Opts             , 0  , 0, 0, 0, "IPv4_Opts             ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv4_Pad              , 0  , 0, 0, 0, "IPv4_Pad              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 2, IPv6_Version          , 4  , 0, 0, 0, "IPv6_Version          ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, IPv6_Traffic_Class    , 8  , 0, 0, 0, "IPv6_Traffic_Class    ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, IPv6_Flow_Label       , 20 , 0, 0, 0, "IPv6_Flow_Label       ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv6_Payload_Len      , 16 , 0, 0, 0, "IPv6_Payload_Len      ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv6_Next_Hdr         , 8  , 0, 0, 0, "IPv6_Next_Hdr         ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv6_Hop_Limit        , 8  , 0, 0, 0, "IPv6_Hop_Limit        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv6_Src_Addr         , 128, 0, 0, 0, "IPv6_Src_Addr         ", 0                , def_srcip6_pattern , Pattern_IPv6, { Fixed_Pattern, 0                , "0.0.0.0.0.0.0.0"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, IPv6_Dest_Addr        , 128, 0, 0, 0, "IPv6_Dest_Addr        ", 0                , def_dstip6_pattern , Pattern_IPv6, { Fixed_Pattern, 0                , "0.0.0.0.0.0.0.0"  , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, TCP_Src_Port          , 16 , 0, 0, 0, "TCP_Src_Port          ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, TCP_Dest_Port         , 16 , 0, 0, 0, "TCP_Dest_Port         ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, TCP_Seq_Num           , 32 , 0, 0, 0, "TCP_Seq_Num           ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, TCP_Ack_Num           , 32 , 0, 0, 0, "TCP_Ack_Num           ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 7, TCP_Data_Offset       , 4  , 0, 0, 0, "TCP_Data_Offset       ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, TCP_Reserved          , 6  , 0, 0, 0, "TCP_Reserved          ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, TCP_Urg               , 1  , 0, 0, 0, "TCP_Urg               ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, TCP_Ack               , 1  , 0, 0, 0, "TCP_Ack               ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, TCP_Psh               , 1  , 0, 0, 0, "TCP_Psh               ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, TCP_Rst               , 1  , 0, 0, 0, "TCP_Rst               ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, TCP_Syn               , 1  , 0, 0, 0, "TCP_Syn               ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, TCP_Fin               , 1  , 0, 0, 0, "TCP_Fin               ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, TCP_Window            , 16 , 0, 0, 0, "TCP_Window            ", 64               , {0x00}             , Integer,      { Fixed_Value  , 64               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, TCP_Csum              , 16 , 0, 0, 0, "TCP_Csum              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, TCP_Urg_Ptr           , 16 , 0, 0, 0, "TCP_Urg_Ptr           ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, TCP_Opts              , 0  , 0, 0, 0, "TCP_Opts              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, TCP_Pad               , 0  , 0, 0, 0, "TCP_Pad               ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, UDP_Src_Port          , 16 , 0, 0, 0, "UDP_Src_Port          ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, UDP_Dest_Port         , 16 , 0, 0, 0, "UDP_Dest_Port         ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, UDP_Len               , 16 , 0, 0, 0, "UDP_Len               ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, UDP_Csum              , 16 , 0, 0, 0, "UDP_Csum              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, ARP_Hw_Type           , 16 , 0, 0, 0, "ARP_Hw_Type           ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, ARP_Proto_Type        , 16 , 0, 0, 0, "ARP_Proto_Type        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, ARP_Hw_Len            , 8  , 0, 0, 0, "ARP_Hw_Len            ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, ARP_Proto_Len         , 8  , 0, 0, 0, "ARP_Proto_Len         ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, ARP_Opcode            , 16 , 0, 0, 0, "ARP_Opcode            ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, ARP_Sender_Hw_Addr    , 48 , 0, 0, 0, "ARP_Sender_Hw_Addr    ", 0                , def_srcmac_pattern , Pattern_MAC,  { Fixed_Pattern, 0                , "00:00:00:00:00:00", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, ARP_Sender_Proto_addr , 32 , 0, 0, 0, "ARP_Sender_Proto_addr ", 0                , def_srcip4_pattern , Pattern_IPv4, { Fixed_Pattern, 0                , "0.0.0.0"          , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, ARP_Target_Hw_Addr    , 48 , 0, 0, 0, "ARP_Target_Hw_Addr    ", 0                , def_dstmac_pattern , Pattern_MAC,  { Fixed_Pattern, 0                , "00:00:00:00:00:00", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, ARP_Target_Proto_Addr , 32 , 0, 0, 0, "ARP_Target_Proto_Addr ", 0                , def_dstip4_pattern , Pattern_IPv4, { Fixed_Pattern, 0                , "0.0.0.0"          , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 2, MPLS_Label            , 20 , 0, 0, 0, "MPLS_Label            ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, MPLS_Exp              , 3  , 0, 0, 0, "MPLS_Exp              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, MPLS_Stack            , 1  , 0, 0, 0, "MPLS_Stack            ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, MPLS_Ttl              , 8  , 0, 0, 0, "MPLS_Ttl              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, VLAN_Tpi              , 16 , 0, 0, 0, "VLAN_Tpi              ", 0x8100           , {0x00}             , Integer,      { Fixed_Value  , 0x8100           , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 2, VLAN_Tci_Pcp          , 3  , 0, 0, 0, "VLAN_Tci_Pcp          ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, VLAN_Tci_Cfi          , 1  , 0, 0, 0, "VLAN_Tci_Cfi          ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 1, VLAN_Vid              , 12 , 0, 0, 0, "VLAN_Vid              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, MAC_Control           , 16 , 0, 0, 0, "MAC_Control           ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, MAC_Control_Opcode    , 16 , 0, 0, 0, "MAC_Control_Opcode    ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Pause_Quanta          , 16 , 0, 0, 0, "Pause_Quanta          ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Priority_En_Vector    , 16 , 0, 0, 0, "Priority_En_Vector    ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Pause_Quanta_0        , 16 , 0, 0, 0, "Pause_Quanta_0        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Pause_Quanta_1        , 16 , 0, 0, 0, "Pause_Quanta_1        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Pause_Quanta_2        , 16 , 0, 0, 0, "Pause_Quanta_2        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Pause_Quanta_3        , 16 , 0, 0, 0, "Pause_Quanta_3        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Pause_Quanta_4        , 16 , 0, 0, 0, "Pause_Quanta_4        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Pause_Quanta_5        , 16 , 0, 0, 0, "Pause_Quanta_5        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Pause_Quanta_6        , 16 , 0, 0, 0, "Pause_Quanta_6        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Pause_Quanta_7        , 16 , 0, 0, 0, "Pause_Quanta_7        ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, FRAME_Len             , 32 , 0, 0, 0, "FRAME_Len             ", 64               , {0x00}             , Integer,      { Fixed_Value  , 64               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, PAYLOAD_Pattern       , 0  , 0, 0, 0, "PAYLOAD_Pattern       ", 0                , {0x00}             , Integer,      { Fixed_Pattern, 0                , "00"               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, STREAM_Traffic_Type   , 32 , 0, 0, 0, "STREAM_Traffic_Type   ", Continuous       , {0x00}             , Integer,      { Fixed_Value  , Continuous       , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, STREAM_Traffic_Control, 32 , 0, 0, 0, "STREAM_Traffic_Control", Stop_After_Stream, {0x00}             , Integer,      { Fixed_Value  , Stop_After_Stream, ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, STREAM_Ipg            , 32 , 0, 0, 0, "STREAM_Ipg            ", 12               , {0x00}             , Integer,      { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, STREAM_Isg            , 32 , 0, 0, 0, "STREAM_Ifg            ", 12               , {0x00}             , Integer,      { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, STREAM_Ibg            , 32 , 0, 0, 0, "STREAM_Ibg            ", 12               , {0x00}             , Integer,      { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, STREAM_Bandwidth      , 32 , 0, 0, 0, "STREAM_Bandwidth      ", 100              , {0x00}             , Integer,      { Fixed_Value  , 100              , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, STREAM_Start_Delay    , 32 , 0, 0, 0, "STREAM_Start_Delay    ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, UDF                   , 0  , 0, 0, 0, "UDF                   ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, META_Len              , 32 , 0, 0, 0, "META_Len              ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, META_Ipg              , 32 , 0, 0, 0, "META_Ipg              ", 12               , {0x00}             , Integer,      { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, META_Preamble         , 64 , 0, 0, 0, "META_Preamble         ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, META_Pad1             , 64 , 0, 0, 0, "META_Pad1             ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, META_Pad2             , 64 , 0, 0, 0, "META_Pad2             ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, META_Pad3             , 64 , 0, 0, 0, "META_Pad3             ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, META_Pad4             , 64 , 0, 0, 0, "META_Pad4             ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, META_Pad5             , 64 , 0, 0, 0, "META_Pad5             ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, META_Pad6             , 64 , 0, 0, 0, "META_Pad6             ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, Zeros_8Bit            , 8  , 0, 0, 0, "Zeros_8Bit            ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
{false, 0, TCP_Total_Len         , 16 , 0, 0, 0, "TCP_Total_Len         ", 0                , {0x00}             , Integer,      { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, {}, 0, 0 }},
};

void signal_handler(int signal) {
    if (signal == SIGABRT) {
        cealog << endl << "<<<< Aborting simulation >>>>" << endl;
    } else {
        cerr << "Unexpected signal " << signal << " received\n";
    }
    exit(EXIT_FAILURE);
}

void print_ftable(vector<cea_field_spec>tbl) {
    stringstream ss;
    ss.setf(ios_base::left);

    for(auto item : tbl) {
        ss << item.name << endl;
    }
    ss << endl;
    cealog << ss.str();
}

// map header type to list of associated fields
map <cea_header_type, vector <cea_field_id>> hfmap = {
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

void print_cdata (unsigned char* tmp, int len) {
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
cea_field_spec get_field(vector<cea_field_spec> tbl, cea_field_id id) {
    auto lit = find_if(tbl.begin(), tbl.end(),
        [&id](const cea_field_spec &item) {
        return (item.id == id); });

    if (lit==tbl.end()) {
        CEA_ERR_MSG("Internal: Unrecognized field identifier: " << id);
        abort();
    }
    return (*lit);
}

//-------------
// Stream Core
//-------------

class cea_stream::core {
public:
    // ctor
    core(string name);

    // dtor
    ~core();

    // Quickly set a fixed value to a field
    void set(cea_field_id id, uint64_t value);

    // Define a complete spec for the generation of a field
    void set(cea_field_id id, cea_gen_spec spec);

    // enable or disable a stream feature
    void set(cea_stream_feature_id feature);

    // Based on user specification of the frame, build a vector of 
    // field ids in the sequence required by the specification
    void gather_fields();

    // The addition of mpls/vlan/llc/snap affects the position of ethertype and
    // length fields. update/insert type or len after arranging all fields in
    // the required sequence
    // TODO check if this is required
    void update_ethertype_and_len();

    // evaluate field length and calculate offset of all fields of this stream
    void build_offsets();
    uint32_t hdr_len;

    // build cseq by parsing fseq and adding only mutable fields
    void filter_mutables();

    // concatenate all fields reuired by the frame spec
    uint32_t splice_fields(unsigned char *buf);

    // build size and payload pattern arrays
    void build_payload_arrays();

    void build_runtime_mark0();
    void build_runtime();

    void build_principal_frame();

    // process the headers and fields and prepare for generation
    void bootstrap();

    // begin generation
    void mutate();

    // print the headers and fields in a tree structure
    void display_stream();

    // Factory reset of the stream core
    void reset();

    // Store the header pointers added to the stream
    // for generation
    vector<cea_header*> headers;
    
    // all_fields will be used to store all the fields added
    // by user by the way of adding headers
    vector<cea_field_spec> all_fields;

    // a copy of the all the default fields and it values
    vector<cea_field_spec> stream_db;

    // store stream properties
    vector<cea_field_spec> properties;
    
    // store user defined fields
    vector<cea_field_spec> udfs;

    void init_properties();

    void convert_string_to_uca(string address, unsigned char *op);
    void convert_mac_to_uca(string address, unsigned char *op);
    void convert_ipv4_to_uca(string address, unsigned char *op);
    void convert_ipv6_to_uca(string address, unsigned char *op);
    unsigned char convert_char_to_int(string hexNumber);
    int convert_nibble_to_int(char digit);

    // mutables will be used to store only those fields that will used during
    // stream generation
    vector<cea_field_spec> mutables;
    vector<cea_field_spec> mutables_bkp;
    // whenthe mutables is populated, copy it to bkp
    // at reset assign bkp to mutables
    // WHY?
    // at the end of the mutation logic, the mutables will be empty
    // so if the user press start button agaon, the mutables will be empty
    // so a bkp version is maintained to restore the content of mutables
    // ELSE
    // everytime the start is pressed we have to reset the stream and bootstrap
    // again. this looks ok, but we need to check if by doing this we will lose
    // any important data when we reset the stream

    // pcap handle for recording
    pcap *txpcap;
    pcap *rxpcap;

    // Prefixture to stream messages
    string stream_name;
    uint32_t stream_id;
    string msg_prefix;

    uint32_t nof_sizes;
    uint32_t hdr_size;
    uint32_t meta_size;
    uint32_t crc_len;
    vector<uint32_t> arof_frm_sizes;
    vector<uint32_t> arof_computed_frm_sizes;
    vector<uint32_t> arof_pl_sizes;

    unsigned char *arr_payl_data;
    unsigned char *arr_rnd_payl_data[CEA_MAX_RND_ARRAYS];
    unsigned char *payload_pattern; // TODO what is this
    uint32_t payload_pattern_size; // TODO what is this?

    // principal frame
    unsigned char *pf;
};

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
    void set(cea_gen_spec spec);

    // The protocol field type that this class represents
    cea_field_id fid;

    // A table of field structs that corresponds to the field identifiers
    // required by this field
    cea_field_spec field;

    // prefixture to field messages
    string field_name;
    string msg_prefix;
};

//-------------
// Field Core
//-------------

class cea_udf::core {
public:
    // ctor
    core();

    // dtor
    ~core();

    // Define a complete spec for the generation of a field
    void set(cea_gen_spec spec);

    // A table of field structs that corresponds to the field identifiers
    // required by this field
    cea_field_spec field;

    // prefixture to field messages
    string field_name;
    string msg_prefix;
};

//-------------
// Header Core
//-------------

class cea_header::core {
public:
    // ctor
    core(cea_header_type hdr);

    // dtor
    ~core();

    // Quickly set a fixed value to a field
    void set(cea_field_id id, uint64_t value);

    // Quickly set a fixed pattern to a field (limited set)
    void set(cea_field_id id, string value);

    // Define a complete spec for the generation of a field
    void set(cea_field_id id, cea_gen_spec spec);

    // The protocol header type that this class represents
    cea_header_type htype;

    // A list of field identifiers that is required by this header 
    vector<cea_field_id> hfids;

    // A table of field structs that corresponds to the field identifiers
    // required by this header
    vector<cea_field_spec> hdrs;

    // copy the un-modified field structs from fdb corresponding to the field
    // identifiers that are required by this header
    void build_hdr_fields();

    // prefixture to header messages
    string header_name;
    string msg_prefix;
};

//-----------
// Port Core
//-----------

class cea_port::core {
public:
    core(string name);

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
    cea_stream *cur_stream;

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

//------------------------------------------------------------------------------
// Field implementation
//------------------------------------------------------------------------------

cea_field::cea_field(cea_field_id id) {
    impl = make_unique<core>(id); 
}

cea_field::core::core(cea_field_id id) {
    fid = id;
    field = get_field(fdb, fid); // copy default values
}

void cea_field::set(uint64_t value) {
    impl->set(value);
}

void cea_field::set(cea_gen_spec spec) {
    impl->set(spec);
}

void cea_field::core::set(uint64_t value) {
    field.spec.value = value;
    field.spec.gen_type = Fixed_Value;
}

void cea_field::core::set(cea_gen_spec spec) {
    field.spec.gen_type     = spec.gen_type;
    field.spec.value        = spec.value;
    field.spec.pattern      = spec.pattern;
    field.spec.step         = spec.step;
    field.spec.min          = spec.min;
    field.spec.max          = spec.max;
    field.spec.count        = spec.count;
    field.spec.repeat       = spec.repeat;
    field.spec.mask         = spec.mask;
    field.spec.seed         = spec.seed;
    field.spec.start        = spec.start;
    field.spec.make_error   = spec.make_error;
    field.spec.value_list   = spec.value_list;
    field.spec.pattern_list = spec.pattern_list;
}

cea_field::~cea_field() = default;
cea_field::core::~core() = default;

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

void cea_stream::set(cea_field_id id, cea_gen_spec spec) {
    impl->set(id, spec);
}

void cea_stream::set(cea_stream_feature_id feature) {
    impl->set(feature);
}

void cea_stream::add_header(cea_header *hdr) {
    hdr->impl->msg_prefix = impl->msg_prefix + "|" + hdr->impl->msg_prefix;
    impl->headers.push_back(hdr);
}

// TODO pending implementation
void cea_stream::add_udf(cea_field *fld) {
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

    // check if id is a property and then add to properties
    auto prop = find_if(properties.begin(), properties.end(),
        [&id](const cea_field_spec &item) {
        return (item.id == id); });

    if (prop != properties.end()) {
        prop->spec.value = value;
        prop->is_mutable = false;
    } else {
        CEA_ERR_MSG("The ID " << id << " does not belong to stream properties");
        abort();
    }
}

void cea_stream::core::set(cea_field_id id, cea_gen_spec spec) {

    // check if id is a property and then add to properties
    auto prop = find_if(properties.begin(), properties.end(),
        [&id](const cea_field_spec &item) {
        return (item.id == id); });

    if (prop != properties.end()) {
        prop->spec = spec;

        if (prop->spec.gen_type != Fixed_Value || prop->spec.gen_type != Fixed_Pattern) {
            prop->is_mutable = true;
        } else {
            prop->is_mutable = false;
        }
    } else {
        CEA_ERR_MSG("The ID " << id << " does not belong to stream properties");
        abort();
    }
}

void cea_stream::core::set(cea_stream_feature_id feature) {
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
uint32_t cea_stream::core::splice_fields(unsigned char *buf) {
    uint32_t offset = 0;
    uint64_t mrg_data = 0;
    uint64_t mrg_len = 0;
    uint64_t mrg_cnt_total = 0;
    uint64_t mrg_cntr = 0;
    bool mrg_start = false;

    for (auto f : all_fields) {
        if(f.merge==0) {
            if (f.type == Integer) {
                cea_memcpy_ntw_byte_order(buf+offset, (char*)&f.def_value, f.len/8);
            }
            else {
                memcpy(buf+offset, f.def_pattern.data(), f.len/8);
                if (f.type == Pattern_IPv4) {
                }
            }
            offset += f.len/8;
        } else {
            if (!mrg_start) {
                mrg_start = true;
                mrg_cnt_total = f.merge + 1;
            }
            mrg_data = (mrg_data << f.len) | f.def_value;
            mrg_len = mrg_len + f.len;
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

void cea_stream::core::bootstrap() {
    gather_fields();
    update_ethertype_and_len();
    build_offsets();
    build_runtime();
    filter_mutables();
    display_stream();
    build_payload_arrays();
    build_principal_frame();
}

void cea_stream::core::gather_fields() {
    all_fields.clear();
    for (auto f : headers) {
        all_fields.insert(
            all_fields.end(),
            f->impl->hdrs.begin(),
            f->impl->hdrs.end()
        );
    }
}

// TODO pending implementation
void cea_stream::core::update_ethertype_and_len() {
}

void cea_stream::core::build_offsets() {
    vector<cea_field_spec>::iterator it;
    for(it=all_fields.begin(); it<all_fields.end(); it++) {
        it->offset = prev(it)->len + prev(it)->offset;
        hdr_len = hdr_len + it->len;
    }
}

void cea_stream::core::filter_mutables() {
    mutables.clear();
    for (auto f : all_fields) {
        if (f.is_mutable) {
            mutables.push_back(f);
        }
    }
    // TODO peniding implementaton: init runtime values

}

void print_field(cea_field_spec item) {
    stringstream ss;
    ss.setf(ios_base::left);

        ss << setw(5) << left << item.id ;
        ss << setw(25) << left << item.name;
        ss << setw(25) << left << item.len;
        ss << setw(25) << left << cea_field_type_name[item.type];;
        if (item.type == Integer)
            ss << setw(25) << left << item.spec.value;
        else
            ss << setw(25) << left << item.spec.pattern;
        ss << setw(25) << left << cea_gen_type_name[item.spec.gen_type];
        ss << endl;
    ss << endl;
    cealog << ss.str();
}

void print(vector<cea_field_spec>tbl) {
    stringstream ss;
    ss.setf(ios_base::left);

    for(auto item : tbl) {
        ss << setw(5) << left << item.id ;
        ss << setw(25) << left << item.name;
        ss << setw(25) << left << item.len;
        ss << setw(25) << left << cea_field_type_name[item.type];;
        if (item.type == Integer)
            ss << setw(25) << left << item.spec.value;
        else
            ss << setw(25) << left << item.spec.pattern;
        ss << setw(25) << left << cea_gen_type_name[item.spec.gen_type];
        ss << endl;
    }
    ss << endl;
    cealog << ss.str();
}

void cea_stream::core::display_stream() {
    for (auto f : headers) {
        cealog << cea_header_name[f->impl->htype] << endl;
        for (auto item : f->impl->hdrs) {
            cealog << "  |--" << item.name << endl;
        }
    }
    print(properties);
    print(mutables);
}

void cea_stream::core::build_payload_arrays() {
    //-------------
    // size arrays
    //-------------
    auto len_item = get_field(properties, FRAME_Len);
    cea_gen_spec spec = len_item.spec;

    nof_sizes = 0;

    switch (spec.gen_type) {
        case Fixed_Value: {
            nof_sizes = 1;
            arof_frm_sizes.resize(nof_sizes);
            arof_computed_frm_sizes.resize(nof_sizes);
            arof_pl_sizes.resize(nof_sizes);
            arof_frm_sizes[0] = spec.value;
            arof_computed_frm_sizes[0] = arof_frm_sizes[0] + meta_size;
            arof_pl_sizes[0] = arof_frm_sizes[0] - (hdr_size - meta_size) - crc_len;
            break;
            }
        case Increment: {
            nof_sizes = ((spec.max - spec.min)/spec.step)+1;
            arof_frm_sizes.resize(nof_sizes);
            arof_computed_frm_sizes.resize(nof_sizes);
            arof_pl_sizes.resize(nof_sizes);
            uint32_t szidx=0;
            for (uint32_t i=spec.min; i<=spec.max; i=i+spec.step) {
                arof_frm_sizes[szidx] = i;
                arof_computed_frm_sizes[szidx] = arof_frm_sizes[szidx] + meta_size;
                arof_pl_sizes[szidx] = arof_frm_sizes[szidx] - (hdr_size - meta_size) - crc_len;
                szidx++;
            }
            break;
            }
        case Decrement: {
            nof_sizes = ((spec.min - spec.max)/spec.step)+1;
            arof_frm_sizes.resize(nof_sizes);
            arof_computed_frm_sizes.resize(nof_sizes);
            arof_pl_sizes.resize(nof_sizes);
            uint32_t szidx=0;
            for (uint32_t i=spec.min; i>=spec.max; i=i-spec.step) {
                arof_frm_sizes[szidx] = i;
                arof_computed_frm_sizes[szidx] = arof_frm_sizes[szidx] + meta_size;
                arof_pl_sizes[szidx] = arof_frm_sizes[szidx] - (hdr_size - meta_size) - crc_len;
                szidx++;
            }
            break;
            }
        case Random: {
            nof_sizes = (spec.max - spec.min) + 1;
            arof_frm_sizes.resize(nof_sizes);
            arof_computed_frm_sizes.resize(nof_sizes);
            arof_pl_sizes.resize(nof_sizes);
            random_device rd;
            mt19937 gen(rd());
            uniform_int_distribution<> distr(spec.max, spec.min);
            uint32_t szidx=0;
            for (uint32_t szidx=spec.min; szidx>spec.max; szidx++) {
                arof_frm_sizes[szidx] = distr(gen);
                arof_computed_frm_sizes[szidx] = arof_frm_sizes[szidx] + meta_size;
                arof_pl_sizes[szidx] = arof_frm_sizes[szidx] - (hdr_size - meta_size) - crc_len;
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
    arr_payl_data = new unsigned char[CEA_MAX_FRAME_SIZE];
    auto pl_item = get_field(properties, PAYLOAD_Pattern);
    cea_gen_spec plspec = pl_item.spec;

    switch (plspec.gen_type) {
        case Random : {
            // create random arrays and fill it with random data
            srand(time(NULL));
            for (uint32_t idx=0; idx<CEA_MAX_RND_ARRAYS; idx++) {
                uint32_t array_size = CEA_MAX_FRAME_SIZE + CEA_RND_ARRAY_SIZE;
                arr_rnd_payl_data[idx] = new unsigned char[array_size];
                for(uint32_t offset=0; offset<array_size; offset++) {
                    int num = rand()%255;
                    memcpy(arr_rnd_payl_data[idx]+offset, (unsigned char*)&num, 1);
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
                    memcpy(arr_payl_data+offset, payload_pattern, payload_pattern_size);
                    offset += payload_pattern_size;
                }
                memcpy(arr_payl_data+offset, payload_pattern, remainder);
            } else {
                memcpy(arr_payl_data+offset, payload_pattern, payload_pattern_size);
            }
            delete [] payload_pattern;
            break;
            }
        case Increment_Byte: {
            uint32_t offset = 0;
            for (uint32_t idx=0; idx<CEA_MAX_FRAME_SIZE/256; idx++) {
                for (uint16_t val=0; val<256; val++) {
                    memcpy(arr_payl_data+offset, (char*)&val, 1);
                    offset++;
                }
            }
            break;
            }
        case Increment_Word: {
            uint32_t offset = 0;
            for (uint32_t idx=0; idx<CEA_MAX_FRAME_SIZE/2; idx++) {
                cea_memcpy_ntw_byte_order(arr_payl_data+offset, (char*)&idx, 2);
                offset += 2;
            }
            break;
            }
        case Decrement_Byte: {
            uint32_t offset = 0;
            for (uint32_t idx=0; idx<CEA_MAX_FRAME_SIZE/256; idx++) {
                for (int16_t val=255; val>=0; val--) {
                    memcpy(arr_payl_data+offset, (char*)&val, 1);
                    offset++;
                }
            }
            break;
            }
        case Decrement_Word: {
            uint32_t offset = 0;
            for (uint32_t val=0xFFFF; val>=0; val--) {
                memcpy(arr_payl_data+offset, (char*)&val, 2);
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

void cea_stream::core::build_runtime() {
    for (auto &m : mutables) {
        switch (m.type) {
            case Integer: {
                switch (m.spec.gen_type) {
                    case Fixed_Value: {
                        m.rt.value = m.spec.value;
                        break;
                        }
                    case Increment: {
                        m.rt.value = m.spec.start;
                        break;
                        }
                    case Decrement: {
                        m.rt.value = m.spec.start;
                        break;
                        }
                    case Value_List: {
                        m.rt.patterns = m.spec.value_list;
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
                switch (m.spec.gen_type) {
                    case Fixed_Pattern: {
                        string tmp_mac_string = m.spec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Increment: {
                        string tmp_mac_string = m.spec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Decrement: {
                        string tmp_mac_string = m.spec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Pattern_List: {
                        m.rt.patterns.resize(0);
                        for (auto val : m.spec.pattern_list) {
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
                switch (m.spec.gen_type) {
                    case Fixed_Pattern: {
                        string tmp_mac_string = m.spec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), '.'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Increment: {
                        string tmp_mac_string = m.spec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Decrement: {
                        string tmp_mac_string = m.spec.pattern;
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                        uint64_t tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.rt.value = tmp_mac;
                        break;
                        }
                    case Pattern_List: {
                        m.rt.patterns.resize(0);
                        for (auto val : m.spec.pattern_list) {
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

// TODO
// assign default value to runtime
// TODO CRITICAL
// when assigning random spec to fields. the test first creates a empty spec,
// sets random and assigns to the field. In this process the default value is
// erased when assigning the empty spec+random to the field
// so when build_runtime is called the default values/patterns are void and
// hence results in error when trying to access default and convert default
// values for patterns
// TODO POSSIBLE SOLUTION: To be decided
//      -> call build_runtime after the principal frame is generated ??
void cea_stream::core::build_runtime_mark0() {
//    for (auto &f : all_fields) {
//        if (f.type == Integer) {
//            f.rt.gen_value = f.spec.value;
//        } else {
//            switch (f.type) {
//                case Pattern_PRE:{
//                    convert_string_to_uca(f.spec.pattern, f.rt.patterns);
//                    break;
//                    }
//                case Pattern_MAC:{
//                    convert_mac_to_uca(f.spec.pattern, f.rt.patterns);
//                    break;
//                    }
//                case Pattern_IPv4:{
//                    convert_ipv4_to_uca(f.spec.pattern, f.rt.patterns);
//                    break;
//                    }
//                case Pattern_IPv6:{
//                    convert_ipv6_to_uca(f.spec.pattern, f.rt.patterns);
//                    break;
//                    }
//                default:{
//                    CEA_ERR_MSG("Invalid Generation type Specified"); // TODO
//                    break;
//                }
//            }    
//        }
//    }
}

// TODO Incomplete implementation
void cea_stream::core::build_principal_frame() {

    // print(all_fields);
    splice_fields(pf);

    auto len_item = get_field(properties, FRAME_Len);
    cea_gen_spec lenspec = len_item.spec;

    auto pl_item = get_field(properties, PAYLOAD_Pattern);
    cea_gen_spec plspec = pl_item.spec;

    uint32_t ploffset = hdr_len/8;

    if (plspec.gen_type == Random)
        memcpy(pf+ploffset, arr_rnd_payl_data[0], lenspec.value);
    else 
        memcpy(pf+ploffset, arr_payl_data, lenspec.value);

    print_cdata(pf, ploffset+lenspec.value);
}

/*
for (auto it=begin(list); it!=end(list)) {
    cout << *it << endl;
    if (*it == 12) {
        list.erase(it);
    } else {
        ++it;
    }
}
*/

// TODO nof frames in outer loop    
// TODO what if there are no mutables
// TODO enclose mutate with perf timers
void cea_stream::core::mutate() {
    for (auto m=begin(mutables); m!=end(mutables); m++) {
        switch(m->type) {
            case Integer: {
                switch(m->spec.gen_type) {
                    case Fixed_Value: {
                        cea_memcpy_ntw_byte_order(pf+m->offset, (char*)&m->spec.value, m->len/8);
                        m->is_mutable = false;
                        mutables.erase(m); m++;
                        break;
                        }
                    case Value_List: {
                        cea_memcpy_ntw_byte_order(pf+m->offset, (char*)&m->rt.patterns[m->rt.idx], m->len/8);
                        if (m->rt.idx == m->rt.patterns.size()-1) {
                            if (m->spec.repeat) {
                                m->rt.idx = 0;
                            } else {
                                mutables.erase(m); m++;
                            }
                        } else {
                            m->rt.idx++;
                        }
                        break;
                        }
                    case Increment: {
                        cea_memcpy_ntw_byte_order(pf+m->offset, (char*)&m->rt.value, m->len/8);
                        if (m->rt.count == m->spec.count) {
                            if (m->spec.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->spec.start;
                            } else {
                                mutables.erase(m); m++;
                            }
                        } else {
                            // TODO check overflow
                            m->rt.value += m->spec.step;
                            m->rt.count++;
                        }
                        break;
                        }
                    case Decrement: {
                        cea_memcpy_ntw_byte_order(pf+m->offset, (char*)&m->rt.value, m->len/8);
                        if (m->rt.count == m->spec.count) {
                            if (m->spec.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->spec.start;
                            } else {
                                mutables.erase(m); m++;
                            }
                        } else {
                            // TODO check underflow
                            m->rt.value -= m->spec.step;
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
                switch(m->spec.gen_type) {
                    case Fixed_Pattern: {
                        cea_memcpy_ntw_byte_order(pf+m->offset, (char*)&m->rt.value, m->len/8);
                        m->is_mutable = false;
                        mutables.erase(m); // m++; // TODO iterator increment
                                           // fails if it is done after the last
                                           // element is deleted, swap erase and
                                           // increment
                        break;
                        }
                    case Pattern_List: {
                        cea_memcpy_ntw_byte_order(pf+m->offset, (char*)&m->rt.patterns[m->rt.idx], m->len/8);
                        if (m->rt.idx == m->rt.patterns.size()-1) {
                            if (m->spec.repeat) {
                                m->rt.idx = 0;
                            } else {
                                mutables.erase(m); // m++;
                            }
                        } else {
                            m->rt.idx++;
                        }
                        break;
                        }
                    case Increment: {
                        cea_memcpy_ntw_byte_order(pf+m->offset, (char*)&m->rt.value, m->len/8);
                        if (m->rt.count == m->spec.count) {
                            if (m->spec.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->spec.start;
                            } else {
                                mutables.erase(m); // m++;
                            }
                        } else {
                            // TODO check overflow
                            m->rt.value += m->spec.step;
                            m->rt.count++;
                        }
                        break;
                        }
                    case Decrement: {
                        cea_memcpy_ntw_byte_order(pf+m->offset, (char*)&m->rt.value, m->len/8);
                        if (m->rt.count == m->spec.count) {
                            if (m->spec.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->spec.start;
                            } else {
                                mutables.erase(m); // m++;
                            }
                        } else {
                            // TODO check underflow
                            m->rt.value -= m->spec.step;
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

void cea_stream::core::init_properties() {
    properties.clear();
    vector<cea_field_id> prop_ids =  hfmap[PROPERTIES];

    for (auto id : prop_ids) {
        auto item = get_field(fdb, id);
        properties.push_back(item);
    }
}

void cea_stream::core::reset() {
    headers.clear();

    // add metadata to headers by default
    // cea_header *meta = new cea_header(META);
    // headers.push_back(meta);

    all_fields.clear();
    stream_db = fdb;
    udfs.clear();
    init_properties();

    // TODO memory leak when reset is done twice in same test
    pf = new unsigned char [CEA_PF_SIZE];

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

//------------------------------------------------------------------------------
// Header implementation
//------------------------------------------------------------------------------

void cea_header::set(cea_field_id id, uint64_t value) {
    impl->set(id, value);
}

void cea_header::set(cea_field_id id, cea_gen_spec spec) {
    impl->set(id, spec);
}

void cea_header::set(cea_field_id id, string value) {
    impl->set(id, value);
}

void cea_header::core::set(cea_field_id id, string value) {
    auto field = find_if(hdrs.begin(), hdrs.end(),
        [&id](const cea_field_spec &item) {
        return (item.id == id); });

    if (field != hdrs.end()) {
        field->spec.pattern = value;
        field->is_mutable = true; // TODO check
    } else {
        CEA_ERR_MSG("The field "
        << cea_trim(fdb[id].name) << " does not belong to the "
        << cea_header_name[htype] << " header");
        abort();
    }
}

void cea_header::core::set(cea_field_id id, uint64_t value) {
    auto field = find_if(hdrs.begin(), hdrs.end(),
        [&id](const cea_field_spec &item) {
        return (item.id == id); });

    if (field != hdrs.end()) {
        field->spec.value = value;
        field->is_mutable = true; // TODO check
    } else {
        CEA_ERR_MSG("The field "
        << cea_trim(fdb[id].name) << " does not belong to the "
        << cea_header_name[htype] << " header");
        abort();
    }
}

void cea_header::core::set(cea_field_id id, cea_gen_spec spec) {
    auto field = find_if(hdrs.begin(), hdrs.end(),
        [&id](const cea_field_spec &item) {
        return (item.id == id); });

    if (field != hdrs.end()) {
        field->spec = spec;
        // if (field->spec.gen_type != Fixed_Value) // TODO check
        field->is_mutable = true;
    } else {
        CEA_ERR_MSG("The field "
        << cea_trim(fdb[id].name) << " does not belong to the "
        << cea_header_name[htype] << " header");
        abort();
    }
}

cea_header::cea_header(cea_header_type hdr) {
    impl = make_unique<core>(hdr); 
}

cea_header::core::core(cea_header_type hdr) {
    header_name = string("Header") + ":" + cea_header_name[hdr];
    msg_prefix = header_name;
    htype = hdr;
    build_hdr_fields();
}

void cea_header::core::build_hdr_fields() {
    hfids.clear();
    hdrs.clear();

    // extract the list of field ids that make up this header
    hfids = hfmap[htype];

    for (auto id : hfids) {
        auto item = get_field(fdb, id);
        hdrs.push_back(item);
    }
}

cea_header::core::~core() = default;

//------------------------------------------------------------------------------
// Port Implementation
//------------------------------------------------------------------------------

void cea_port::core::reset() {
    msg_prefix = port_name;
}

void cea_port::core::worker() {
    vector<cea_stream*>::iterator it;

    for (it = streamq.begin(); it != streamq.end(); it++) {
        cur_stream = *it;
        cur_stream->impl->bootstrap();
        cur_stream->impl->mutate();
    }
}

void cea_port::core::start_worker() {
    worker_tid = thread(&cea_port::core::worker, this);
    char name[16];
    sprintf(name, "worker_%d", port_id);
    pthread_setname_np(worker_tid.native_handle(), name);
}

cea_port::~cea_port() = default;

cea_port::cea_port(string name) {
    impl = make_unique<core>(name);
}

cea_port::core::core(string name) {
    port_id = cea::port_id;
    cea::port_id++;
    port_name = name + ":" + to_string(port_id);
    reset();
    CEA_MSG("Proxy created with name=" << name << " and id=" << port_id);
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

//------------------------------------------------------------------------------
// Testbench Implementation
//------------------------------------------------------------------------------

cea_testbench::core::~core() = default;

cea_testbench::cea_testbench() {
    impl = make_unique<core>();
}

cea_testbench::~cea_testbench() = default;

cea_testbench::core::core() {
}
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

//------------------------------------------------------------------------------
// Udf implementation
//------------------------------------------------------------------------------

cea_udf::cea_udf() {
    impl = make_unique<core>(); 
}

cea_udf::core::core() {
    field = {};
}

void cea_udf::set(cea_gen_spec spec) {
    impl->set(spec);
}

void cea_udf::core::set(cea_gen_spec spec) {
    field.spec.gen_type     = spec.gen_type;
    field.spec.value        = spec.value;
    field.spec.pattern      = spec.pattern;
    field.spec.step         = spec.step;
    field.spec.min          = spec.min;
    field.spec.max          = spec.max;
    field.spec.count        = spec.count;
    field.spec.repeat       = spec.repeat;
    field.spec.mask         = spec.mask;
    field.spec.seed         = spec.seed;
    field.spec.start        = spec.start;
    field.spec.make_error   = spec.make_error;
    field.spec.value_list   = spec.value_list;
    field.spec.pattern_list = spec.pattern_list;
}

cea_udf::~cea_udf() = default;
cea_udf::core::~core() = default;

} // namespace
