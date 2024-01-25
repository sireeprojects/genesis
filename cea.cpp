/*
NOTES:
marco CEA_DEBUG - to include debug code and to generate debug library
THINGS TO DO:
*/

#include <thread>
#include <iomanip>
#include <algorithm>
#include <cstring>
#include <map>
#include <cmath>
#include <fstream>
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
// TODO: This will become a problem in multi-process mode
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
    Pattern
};

enum cea_stream_add_type {
    Header,
    Field
};

struct cea_runtime {
    uint64_t value_in_range;
    uint64_t gen_value;
    string gen_pattern;
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
{false, 0, MAC_Preamble          , 64 , 0, 0, 0, "MAC_Preamble          ", Integer, { Fixed_Value  , 0x55555555555555d, ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, MAC_Dest_Addr         , 48 , 0, 0, 0, "MAC_Dest_Addr         ", Integer, { Fixed_Pattern, 0                , "00:00:00:00:00:00", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, MAC_Src_Addr          , 48 , 0, 0, 0, "MAC_Src_Addr          ", Integer, { Fixed_Pattern, 0                , "00:00:00:00:00:00", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, MAC_Len               , 16 , 0, 0, 0, "MAC_Len               ", Integer, { Fixed_Value  , 46               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, MAC_Ether_Type        , 16 , 0, 0, 0, "MAC_Ether_Type        ", Integer, { Fixed_Value  , 0x0800           , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, MAC_Fcs               , 32 , 0, 0, 0, "MAC_Fcs               ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, LLC_Dsap              , 8  , 0, 0, 0, "LLC_Dsap              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, LLC_Ssap              , 8  , 0, 0, 0, "LLC_Ssap              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, LLC_Control           , 8  , 0, 0, 0, "LLC_Control           ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, SNAP_Oui              , 24 , 0, 0, 0, "SNAP_Oui              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, SNAP_Pid              , 16 , 0, 0, 0, "SNAP_Pid              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, IPv4_Version          , 4  , 0, 0, 0, "IPv4_Version          ", Integer, { Fixed_Value  , 4                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, IPv4_IHL              , 4  , 0, 0, 0, "IPv4_IHL              ", Integer, { Fixed_Value  , 5                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv4_Tos              , 8  , 0, 0, 0, "IPv4_Tos              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv4_Total_Len        , 16 , 0, 0, 0, "IPv4_Total_Len        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv4_Id               , 16 , 0, 0, 0, "IPv4_Id               ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, IPv4_Flags            , 3  , 0, 0, 0, "IPv4_Flags            ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, IPv4_Frag_Offset      , 13 , 0, 0, 0, "IPv4_Frag_Offset      ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv4_TTL              , 8  , 0, 0, 0, "IPv4_TTL              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv4_Protocol         , 8  , 0, 0, 0, "IPv4_Protocol         ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv4_Hdr_Csum         , 16 , 0, 0, 0, "IPv4_Hdr_Csum         ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv4_Src_Addr         , 32 , 0, 0, 0, "IPv4_Src_Addr         ", Integer, { Fixed_Pattern, 0                , "0.0.0.0"          , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv4_Dest_Addr        , 32 , 0, 0, 0, "IPv4_Dest_Addr        ", Integer, { Fixed_Pattern, 0                , "0.0.0.0"          , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv4_Opts             , 0  , 0, 0, 0, "IPv4_Opts             ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv4_Pad              , 0  , 0, 0, 0, "IPv4_Pad              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 2, IPv6_Version          , 4  , 0, 0, 0, "IPv6_Version          ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, IPv6_Traffic_Class    , 8  , 0, 0, 0, "IPv6_Traffic_Class    ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, IPv6_Flow_Label       , 20 , 0, 0, 0, "IPv6_Flow_Label       ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv6_Payload_Len      , 16 , 0, 0, 0, "IPv6_Payload_Len      ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv6_Next_Hdr         , 8  , 0, 0, 0, "IPv6_Next_Hdr         ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv6_Hop_Limit        , 8  , 0, 0, 0, "IPv6_Hop_Limit        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv6_Src_Addr         , 128, 0, 0, 0, "IPv6_Src_Addr         ", Integer, { Fixed_Pattern, 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, IPv6_Dest_Addr        , 128, 0, 0, 0, "IPv6_Dest_Addr        ", Integer, { Fixed_Pattern, 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, TCP_Src_Port          , 16 , 0, 0, 0, "TCP_Src_Port          ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, TCP_Dest_Port         , 16 , 0, 0, 0, "TCP_Dest_Port         ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, TCP_Seq_Num           , 32 , 0, 0, 0, "TCP_Seq_Num           ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, TCP_Ack_Num           , 32 , 0, 0, 0, "TCP_Ack_Num           ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 7, TCP_Data_Offset       , 4  , 0, 0, 0, "TCP_Data_Offset       ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, TCP_Reserved          , 6  , 0, 0, 0, "TCP_Reserved          ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, TCP_Urg               , 1  , 0, 0, 0, "TCP_Urg               ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, TCP_Ack               , 1  , 0, 0, 0, "TCP_Ack               ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, TCP_Psh               , 1  , 0, 0, 0, "TCP_Psh               ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, TCP_Rst               , 1  , 0, 0, 0, "TCP_Rst               ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, TCP_Syn               , 1  , 0, 0, 0, "TCP_Syn               ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, TCP_Fin               , 1  , 0, 0, 0, "TCP_Fin               ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, TCP_Window            , 16 , 0, 0, 0, "TCP_Window            ", Integer, { Fixed_Value  , 64               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, TCP_Csum              , 16 , 0, 0, 0, "TCP_Csum              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, TCP_Urg_Ptr           , 16 , 0, 0, 0, "TCP_Urg_Ptr           ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, TCP_Opts              , 0  , 0, 0, 0, "TCP_Opts              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, TCP_Pad               , 0  , 0, 0, 0, "TCP_Pad               ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, UDP_Src_Port          , 16 , 0, 0, 0, "UDP_Src_Port          ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, UDP_Dest_Port         , 16 , 0, 0, 0, "UDP_Dest_Port         ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, UDP_Len               , 16 , 0, 0, 0, "UDP_Len               ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, UDP_Csum              , 16 , 0, 0, 0, "UDP_Csum              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, ARP_Hw_Type           , 16 , 0, 0, 0, "ARP_Hw_Type           ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, ARP_Proto_Type        , 16 , 0, 0, 0, "ARP_Proto_Type        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, ARP_Hw_Len            , 8  , 0, 0, 0, "ARP_Hw_Len            ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, ARP_Proto_Len         , 8  , 0, 0, 0, "ARP_Proto_Len         ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, ARP_Opcode            , 16 , 0, 0, 0, "ARP_Opcode            ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, ARP_Sender_Hw_Addr    , 48 , 0, 0, 0, "ARP_Sender_Hw_Addr    ", Integer, { Fixed_Pattern, 0                , "00:00:00:00:00:00", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, ARP_Sender_Proto_addr , 32 , 0, 0, 0, "ARP_Sender_Proto_addr ", Integer, { Fixed_Pattern, 0                , "0.0.0.0"          , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, ARP_Target_Hw_Addr    , 48 , 0, 0, 0, "ARP_Target_Hw_Addr    ", Integer, { Fixed_Pattern, 0                , "00:00:00:00:00:00", 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, ARP_Target_Proto_Addr , 32 , 0, 0, 0, "ARP_Target_Proto_Addr ", Integer, { Fixed_Pattern, 0                , "0.0.0.0"          , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 2, MPLS_Label            , 20 , 0, 0, 0, "MPLS_Label            ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, MPLS_Exp              , 3  , 0, 0, 0, "MPLS_Exp              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, MPLS_Stack            , 1  , 0, 0, 0, "MPLS_Stack            ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, MPLS_Ttl              , 8  , 0, 0, 0, "MPLS_Ttl              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, VLAN_Tpi              , 16 , 0, 0, 0, "VLAN_Tpi              ", Integer, { Fixed_Value  , 0x8100           , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 2, VLAN_Tci_Pcp          , 3  , 0, 0, 0, "VLAN_Tci_Pcp          ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, VLAN_Tci_Cfi          , 1  , 0, 0, 0, "VLAN_Tci_Cfi          ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 1, VLAN_Vid              , 12 , 0, 0, 0, "VLAN_Vid              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, MAC_Control           , 16 , 0, 0, 0, "MAC_Control           ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, MAC_Control_Opcode    , 16 , 0, 0, 0, "MAC_Control_Opcode    ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Pause_Quanta          , 16 , 0, 0, 0, "Pause_Quanta          ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Priority_En_Vector    , 16 , 0, 0, 0, "Priority_En_Vector    ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Pause_Quanta_0        , 16 , 0, 0, 0, "Pause_Quanta_0        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Pause_Quanta_1        , 16 , 0, 0, 0, "Pause_Quanta_1        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Pause_Quanta_2        , 16 , 0, 0, 0, "Pause_Quanta_2        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Pause_Quanta_3        , 16 , 0, 0, 0, "Pause_Quanta_3        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Pause_Quanta_4        , 16 , 0, 0, 0, "Pause_Quanta_4        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Pause_Quanta_5        , 16 , 0, 0, 0, "Pause_Quanta_5        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Pause_Quanta_6        , 16 , 0, 0, 0, "Pause_Quanta_6        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Pause_Quanta_7        , 16 , 0, 0, 0, "Pause_Quanta_7        ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, FRAME_Len             , 32 , 0, 0, 0, "FRAME_Len             ", Integer, { Fixed_Value  , 64               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, PAYLOAD_Pattern       , 0  , 0, 0, 0, "PAYLOAD_Pattern       ", Integer, { Fixed_Value  , 0                , "00"               , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, STREAM_Traffic_Type   , 32 , 0, 0, 0, "STREAM_Traffic_Type   ", Integer, { Fixed_Value  , Continuous       , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, STREAM_Traffic_Control, 32 , 0, 0, 0, "STREAM_Traffic_Control", Integer, { Fixed_Value  , Stop_After_Stream, ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, STREAM_Ipg            , 32 , 0, 0, 0, "STREAM_Ipg            ", Integer, { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, STREAM_Isg            , 32 , 0, 0, 0, "STREAM_Ifg            ", Integer, { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, STREAM_Ibg            , 32 , 0, 0, 0, "STREAM_Ibg            ", Integer, { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, STREAM_Bandwidth      , 32 , 0, 0, 0, "STREAM_Bandwidth      ", Integer, { Fixed_Value  , 100              , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, STREAM_Start_Delay    , 32 , 0, 0, 0, "STREAM_Start_Delay    ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, UDF                   , 0  , 0, 0, 0, "UDF                   ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, META_Len              , 32 , 0, 0, 0, "META_Len              ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, META_Ipg              , 32 , 0, 0, 0, "META_Ipg              ", Integer, { Fixed_Value  , 12               , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, META_Preamble         , 64 , 0, 0, 0, "META_Preamble         ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, META_Pad1             , 64 , 0, 0, 0, "META_Pad1             ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, META_Pad2             , 64 , 0, 0, 0, "META_Pad2             ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, META_Pad3             , 64 , 0, 0, 0, "META_Pad3             ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, META_Pad4             , 64 , 0, 0, 0, "META_Pad4             ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, META_Pad5             , 64 , 0, 0, 0, "META_Pad5             ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, META_Pad6             , 64 , 0, 0, 0, "META_Pad6             ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, Zeros_8Bit            , 8  , 0, 0, 0, "Zeros_8Bit            ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
{false, 0, TCP_Total_Len         , 16 , 0, 0, 0, "TCP_Total_Len         ", Integer, { Fixed_Value  , 0                , ""                 , 0, 0, 0, 0, 0, 0, 0, 0, 0, {}, {} }, { 0, 0, "", 0, 0 }},
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
    "Increment_Bytes",
    "Decrement_Byte",
    "Increment_Word",
    "Decrement_Word",
    "Continuous",
    "Bursty",
    "Stop_After_Stream",
    "Goto_Next_Stream"
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
            idx++;
        }
        s << endl;
    }
    for (int x=idx; x<len; x++) {
       s<<noshowbase<<setw(2)<<setfill('0') <<hex<<uint16_t(tmp[idx])<<" ";
       idx++;
    }
    cout << "PKT Data :" << endl << s.str()<<endl;
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
    void update_ethertype_and_len();

    // evaluate field length and calculate offset of all fields of this stream
    void build_offsets();

    // build cseq by parsing fseq and adding only mutable fields
    void filter_mutables();

    // concatenate all fields reuired by the frame spec
    uint32_t splice_fields(unsigned char *buf);

    // calculate frame, txn sizes based on user inputs
    void compute_mutation_sizes();

    // build size and payload pattern arrays
    void build_payload_arrays();

    void build_principal_frame();

    // process the headers and fields and prepare for generation
    void bootstrap();

    // begin generation
    void mutate();

    // print the headers and fields in a tree structure
    void display_stream();

    // Factory reset of the stream core
    void reset();

    // TODO not require, remove after confirmation
    vector<cea_stream_add_type> hf_sequencer;
    
    // Store the header pointers added to the stream
    // for generation
    vector<cea_header*> headers;
    
    // Store the field pointers added to the stream
    // for generation
    vector<cea_field*> fields;

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

    // mutables will be used to store only those fields that will used during
    // stream generation
    vector<cea_field_spec> mutables;

    // pcap handle for recording
    pcap *txpcap;
    pcap *rxpcap;

    // Prefixture to stream messages
    string stream_name;
    uint32_t stream_id;
    string msg_prefix;

    uint32_t nof_szs;
    vector<uint32_t> vec_frm_szs;
    vector<uint32_t> vec_txn_szs;
    vector<uint32_t> vec_payl_szs;

    vector<unsigned char> arr_payl_data;
    vector<unsigned char> arr_rnd_payl_data[10]; // TODO make 10 configurable

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

    // Define a complete spec for the generation of a field
    void set(cea_field_id id, cea_gen_spec spec);

    // The protocol header type that this class represents
    cea_header_type htype;

    // A list of field identifiers that is required by this header 
    vector<cea_field_id> hfids;

    // A table of field structs that corresponds to the field identifiers
    // required by this header
    vector<cea_field_spec> htable;

    // copy the un-modified field structs from fdb corresponding to the field
    // identifiers that are required by this header
    void build_htable();

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
    impl->hf_sequencer.push_back(Header);
}

void cea_stream::add_field(cea_field *fld) {
    fld->impl->msg_prefix = impl->msg_prefix + "|" + fld->impl->msg_prefix;
    impl->fields.push_back(fld);
    impl->hf_sequencer.push_back(Field);
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
        if (prop->spec.gen_type != Fixed_Value) {
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

void cea_stream::core::gather_fields() {
    all_fields.clear();
    for (auto f : headers) {
        all_fields.insert(
            all_fields.end(),
            f->impl->htable.begin(),
            f->impl->htable.end()
        );
    }
}

void cea_stream::core::update_ethertype_and_len() {
// TODO
}

void cea_stream::core::build_offsets() {
    vector<cea_field_spec>::iterator it;
    for(it=all_fields.begin(); it<all_fields.end(); it++) {
        it->offset = prev(it)->len + prev(it)->offset;
    }
}

void cea_stream::core::filter_mutables() {
    mutables.clear();
    for (auto f : all_fields) {
        if (f.is_mutable) {
            mutables.push_back(f);
        }
    }
    // TODO init runtime values
}

// TODO: Pending verification
uint32_t cea_stream::core::splice_fields(unsigned char *buf) {
    uint32_t offset = 0;
    uint64_t mrg_data = 0;
    uint64_t mrg_len = 0;

    for (auto f : all_fields) {
        if(f.merge==0) {
            cea_memcpy_ntw_byte_order(buf+offset, (char*)&f.spec.value, f.len/8);
            offset += f.len/8;
        } else {
            mrg_data = (mrg_data << f.len) | f.spec.value;
            mrg_len = mrg_len + f.len;
            for (uint32_t mcntr=0; mcntr<f.merge; mcntr++) {
                mrg_data = (mrg_data << f.len) | f.spec.value;
                mrg_len = mrg_len + f.len;
            }
            cea_memcpy_ntw_byte_order(buf+offset, (char*)&mrg_data, mrg_len/8);
            offset += mrg_len/8;
        }
    }
    return offset;
}

// TODO
void cea_stream::core::compute_mutation_sizes() {
}

// TODO
void cea_stream::core::build_payload_arrays() {
}

// TODO
void cea_stream::core::build_principal_frame() {
}


void cea_stream::core::bootstrap() {
    gather_fields();
    update_ethertype_and_len();
    build_offsets();
    filter_mutables();
    display_stream();
    compute_mutation_sizes();
    build_payload_arrays();
}

void cea_stream::core::mutate() {
}

void cea_stream::core::display_stream() {
    for (auto f : headers) {
        cealog << cea_header_name[f->impl->htype] << endl;
        for (auto item : f->impl->htable) {
            cealog << "  |--" << item.name << endl;
        }
    }
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
    fields.clear();
    hf_sequencer.clear();
    all_fields.clear();
    stream_db = fdb;
    udfs.clear();
    init_properties();
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

void cea_header::core::set(cea_field_id id, uint64_t value) {
    auto field = find_if(htable.begin(), htable.end(),
        [&id](const cea_field_spec &item) {
        return (item.id == id); });

    if (field != htable.end()) {
        field->spec.value = value;
        field->is_mutable = false;
    } else {
        CEA_ERR_MSG("The field "
        << cea_trim(fdb[id].name) << " does not belong to the "
        << cea_header_name[htype] << " header");
        abort();
    }
}

void cea_header::core::set(cea_field_id id, cea_gen_spec spec) {
    auto field = find_if(htable.begin(), htable.end(),
        [&id](const cea_field_spec &item) {
        return (item.id == id); });

    if (field != htable.end()) {
        field->spec = spec;
        if (field->spec.gen_type != Fixed_Value)
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
    build_htable();
}

void cea_header::core::build_htable() {
    hfids.clear();
    htable.clear();

    // extract the list of field ids that make up this header
    hfids = hfmap[htype];

    for (auto id : hfids) {
        auto item = get_field(fdb, id);
        htable.push_back(item);
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
// TODO
}

void cea_port::core::start() {
    start_worker();
}

void cea_port::core::stop() {
// TODO
}

void cea_port::core::pause() {
// TODO
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
// TODO
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
