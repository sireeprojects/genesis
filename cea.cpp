/*
NOTES:
CEA_DEBUG - to include debug code and to generate debug library

THINGS TO DO:
- Support for multiprocess
        proxy_id is taken from global variable. proxies created in other 
        machines will have duplicate proxy_id values

- stream and proxy id should be 3 digits be default

- add leading space when printing stream and proxy messages (optional/fancy)

- new stream function get_field_property to replace the following
        is touched, is_merge and value_of
        uint64_t get_field_property(Property Name, field id)
        string get_field_property(Property Name, field id)

- stream interleaving support        
        stream: add context switch variables.
                current generation context will be stored in stream
        proxy: add funtions 
               conext switch
                    |- save_context
                    |- restore_context
        note: saving and restoring context incur performance penalty
              try some other idea
              idea: store context in stream itself instead of storing 
                    in proxy. then context switch means just pointing to 
                    other stream

- Optimize compute_ udp and tcp csum calculation. avoid payload copy

- dump python script
        how to use python to start test
        in 3p mode, the test is first started and then the tcl script is run
        python mode should be like c++ mode where the test should be invoked
        by python script. it should not be like, first start the c++ test and
        then run the python script. if this is the case python script cannot
        be added to regression

- Unix socket class, IP Socket class and socker server class
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

#include "cea.h"

using namespace std;
using namespace chrono;

//------------------------------------------------------------------------------
// Global properties
//------------------------------------------------------------------------------

// maximum supported frame size from MAC dest addr to MAC crc (16KB)
#define CEA_MAX_FRAME_SIZE 16384

// size of scratchpad buffer
#define CEA_SCRATCHPAD_SIZE 256

//------------------------------------------------------------------------------
// Messaging 
//------------------------------------------------------------------------------

// Used for mandatory messages. Cannot be disabled in debug mode
#define CEA_MSG(msg) { \
    stringstream s; \
    s << msg; \
    cealog << msg_prefix << string(__FUNCTION__) << ": " <<  s.str() << endl; \
}

// Enabled only in debug mode
#ifdef CEA_DEBUG
    #define CEA_DBG(msg) { CEA_MSG(msg) }
#else
    #define CEA_DBG(msg) {}
#endif

//------------------------------------------------------------------------------
// 1GB Hugepage for frame buffer (shared memory)
//------------------------------------------------------------------------------

#define LENGTH (1UL*1024*1024*1024)
#define PROTECTION (PROT_READ | PROT_WRITE)
#define ADDR (void *)(0x0UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)

//------------------------------------------------------------------------------
// CRC32
//------------------------------------------------------------------------------

const uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
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

namespace cea {

vector<cea_field> flds = {
//----------------------------------------------------------------------------------------------------------------------------------------------------
// Toc     Mrg Added    Stack Id                        Len       Offset Modifier Val                  Start Stop Step Rpt Name
//----------------------------------------------------------------------------------------------------------------------------------------------------
{  false,  0,  false,   0,    FRAME_Type               ,0,        0,     Fixed,   ETH_V2,              0,    0,   0,   0,  "FRAME_Type             "},
{  false,  0,  false,   0,    FRAME_Len                ,0,        0,     Fixed,   64,                  0,    0,   0,   0,  "FRAME_Len              "},
{  false,  0,  false,   0,    Network_Hdr              ,0,        0,     Fixed,   IPv4,                0,    0,   0,   0,  "Network_Hdr            "},
{  false,  0,  false,   0,    Transport_Hdr            ,0,        0,     Fixed,   UDP,                 0,    0,   0,   0,  "Transport_Hdr          "},
{  false,  0,  false,   0,    VLAN_Tag                 ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_Tag               "},
{  false,  0,  false,   0,    MPLS_Hdr                 ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_Hdr               "},
{  false,  0,  false,   0,    MAC_Preamble             ,64,       0,     Fixed,   0x55555555555555d5,  0,    0,   0,   0,  "MAC_Preamble           "},
{  false,  0,  false,   0,    MAC_Dest_Addr            ,48,       0,     Fixed,   0x112233445566,      0,    0,   0,   0,  "MAC_Dest_Addr          "},
{  false,  0,  false,   0,    MAC_Src_Addr             ,48,       0,     Fixed,   0xaabbccddeeff,      0,    0,   0,   0,  "MAC_Src_Addr           "},
{  false,  0,  false,   0,    MAC_Len                  ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "MAC_Len                "},
{  false,  0,  false,   0,    MAC_Ether_Type           ,16,       0,     Fixed,   0x0800,              0,    0,   0,   0,  "MAC_Ether_Type         "},
{  false,  0,  false,   0,    MAC_Fcs                  ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "MAC_Fcs                "},
{  false,  0,  false,   0,    LLC_Dsap                 ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "LLC_Dsap               "},
{  false,  0,  false,   0,    LLC_Ssap                 ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "LLC_Ssap               "},
{  false,  0,  false,   0,    LLC_Control              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "LLC_Control            "},
{  false,  0,  false,   0,    SNAP_Oui                 ,24,       0,     Fixed,   0,                   0,    0,   0,   0,  "SNAP_Oui               "},
{  false,  0,  false,   0,    SNAP_Pid                 ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "SNAP_Pid               "},
{  false,  1,  false,   0,    IPv4_Version             ,4,        0,     Fixed,   4,                   0,    0,   0,   0,  "IPv4_Version           "},
{  false,  1,  false,   0,    IPv4_IHL                 ,4,        0,     Fixed,   5,                   0,    0,   0,   0,  "IPv4_IHL               "},
{  false,  0,  false,   0,    IPv4_Tos                 ,8,        0,     Fixed,   0xc0,                0,    0,   0,   0,  "IPv4_Tos               "},
{  false,  0,  false,   0,    IPv4_Total_Len           ,16,       0,     Fixed,   0x33,                0,    0,   0,   0,  "IPv4_Total_Len         "},
{  false,  0,  false,   0,    IPv4_Id                  ,16,       0,     Fixed,   0xaabb,              0,    0,   0,   0,  "IPv4_Id                "},
{  false,  1,  false,   0,    IPv4_Flags               ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Flags             "},
{  false,  1,  false,   0,    IPv4_Frag_Offset         ,13,       0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Frag_Offset       "},
{  false,  0,  false,   0,    IPv4_TTL                 ,8,        0,     Fixed,   10,                  0,    0,   0,   0,  "IPv4_TTL               "},
{  false,  0,  false,   0,    IPv4_Protocol            ,8,        0,     Fixed,   6,                   0,    0,   0,   0,  "IPv4_Protocol          "},
{  false,  0,  false,   0,    IPv4_Hdr_Csum            ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Hdr_Csum          "},
{  false,  0,  false,   0,    IPv4_Src_Addr            ,32,       0,     Fixed,   0x11223344,          0,    0,   0,   0,  "IPv4_Src_Addr          "},
{  false,  0,  false,   0,    IPv4_Dest_Addr           ,32,       0,     Fixed,   0xaabbccdd,          0,    0,   0,   0,  "IPv4_Dest_Addr         "},
{  false,  0,  false,   0,    IPv4_Opts                ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Opts              "},
{  false,  0,  false,   0,    IPv4_Pad                 ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Pad               "},
{  false,  2,  false,   0,    IPv6_Version             ,4,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Version           "},
{  false,  1,  false,   0,    IPv6_Traffic_Class       ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Traffic_Class     "},
{  false,  1,  false,   0,    IPv6_Flow_Label          ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Flow_Label        "},
{  false,  0,  false,   0,    IPv6_Payload_Len         ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Payload_Len       "},
{  false,  0,  false,   0,    IPv6_Next_Hdr            ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Next_Hdr          "},
{  false,  0,  false,   0,    IPv6_Hop_Limit           ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Hop_Limit         "},
{  false,  0,  false,   0,    IPv6_Src_Addr            ,128,      0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Src_Addr          "},
{  false,  0,  false,   0,    IPv6_Dest_Addr           ,128,      0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Dest_Addr         "},
{  false,  0,  false,   0,    TCP_Src_Port             ,16,       0,     Fixed,   1234,                0,    0,   0,   0,  "TCP_Src_Port           "},
{  false,  0,  false,   0,    TCP_Dest_Port            ,16,       0,     Fixed,   5678,                0,    0,   0,   0,  "TCP_Dest_Port          "},
{  false,  0,  false,   0,    TCP_Seq_Num              ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Seq_Num            "},
{  false,  0,  false,   0,    TCP_Ack_Num              ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Ack_Num            "},
{  false,  7,  false,   0,    TCP_Data_Offset          ,4,        0,     Fixed,   5,                   0,    0,   0,   0,  "TCP_Data_Offset        "},
{  false,  1,  false,   0,    TCP_Reserved             ,6,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Reserved           "},
{  false,  1,  false,   0,    TCP_Urg                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Urg                "},
{  false,  1,  false,   0,    TCP_Ack                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Ack                "},
{  false,  1,  false,   0,    TCP_Psh                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Psh                "},
{  false,  1,  false,   0,    TCP_Rst                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Rst                "},
{  false,  1,  false,   0,    TCP_Syn                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Syn                "},
{  false,  1,  false,   0,    TCP_Fin                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Fin                "},
{  false,  0,  false,   0,    TCP_Window               ,16,       0,     Fixed,   64,                  0,    0,   0,   0,  "TCP_Window             "},
{  false,  0,  false,   0,    TCP_Csum                 ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Csum               "},
{  false,  0,  false,   0,    TCP_Urg_Ptr              ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Urg_Ptr            "},
{  false,  0,  false,   0,    TCP_Opts                 ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Opts               "},
{  false,  0,  false,   0,    TCP_Pad                  ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Pad                "},
{  false,  0,  false,   0,    UDP_Src_Port             ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Src_Port           "},
{  false,  0,  false,   0,    UDP_Dest_Port            ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Dest_Port          "},
{  false,  0,  false,   0,    UDP_Len                  ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Len                "},
{  false,  0,  false,   0,    UDP_Csum                 ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Csum               "},
{  false,  0,  false,   0,    ARP_Hw_Type              ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Hw_Type            "},
{  false,  0,  false,   0,    ARP_Proto_Type           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Proto_Type         "},
{  false,  0,  false,   0,    ARP_Hw_Len               ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Hw_Len             "},
{  false,  0,  false,   0,    ARP_Proto_Len            ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Proto_Len          "},
{  false,  0,  false,   0,    ARP_Opcode               ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Opcode             "},
{  false,  0,  false,   0,    ARP_Sender_Hw_Addr       ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Sender_Hw_Addr     "},
{  false,  0,  false,   0,    ARP_Sender_Proto_addr    ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Sender_Proto_addr  "},
{  false,  0,  false,   0,    ARP_Target_Hw_Addr       ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Target_Hw_Addr     "},
{  false,  0,  false,   0,    ARP_Target_Proto_Addr    ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Target_Proto_Addr  "},
{  false,  0,  false,   0,    PAYLOAD_Type             ,46,       0,     Fixed,   0,                   0,    0,   0,   0,  "PAYLOAD_Type           "},
{  false,  0,  false,   0,    PAYLOAD_Len              ,46,       0,     Fixed,   0,                   0,    0,   0,   0,  "PAYLOAD_Len            "},
{  false,  0,  false,   0,    UDF1                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF1                   "},
{  false,  0,  false,   0,    UDF2                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF2                   "},
{  false,  0,  false,   0,    UDF3                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF3                   "},
{  false,  0,  false,   0,    UDF4                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF4                   "},
{  false,  0,  false,   0,    UDF5                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF5                   "},
{  false,  0,  false,   0,    UDF6                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF6                   "},
{  false,  0,  false,   0,    UDF7                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF7                   "},
{  false,  0,  false,   0,    UDF8                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF8                   "},
{  false,  2,  false,   1,    MPLS_01_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_01_Label          "},
{  false,  1,  false,   1,    MPLS_01_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_01_Exp            "},
{  false,  1,  false,   1,    MPLS_01_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_01_Stack          "},
{  false,  0,  false,   1,    MPLS_01_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_01_Ttl            "},
{  false,  2,  false,   2,    MPLS_02_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_02_Label          "},
{  false,  1,  false,   2,    MPLS_02_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_02_Exp            "},
{  false,  1,  false,   2,    MPLS_02_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_02_Stack          "},
{  false,  0,  false,   2,    MPLS_02_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_02_Ttl            "},
{  false,  2,  false,   3,    MPLS_03_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_03_Label          "},
{  false,  1,  false,   3,    MPLS_03_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_03_Exp            "},
{  false,  1,  false,   3,    MPLS_03_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_03_Stack          "},
{  false,  0,  false,   3,    MPLS_03_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_03_Ttl            "},
{  false,  2,  false,   4,    MPLS_04_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_04_Label          "},
{  false,  1,  false,   4,    MPLS_04_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_04_Exp            "},
{  false,  1,  false,   4,    MPLS_04_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_04_Stack          "},
{  false,  0,  false,   4,    MPLS_04_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_04_Ttl            "},
{  false,  2,  false,   5,    MPLS_05_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_05_Label          "},
{  false,  1,  false,   5,    MPLS_05_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_05_Exp            "},
{  false,  1,  false,   5,    MPLS_05_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_05_Stack          "},
{  false,  0,  false,   5,    MPLS_05_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_05_Ttl            "},
{  false,  2,  false,   6,    MPLS_06_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_06_Label          "},
{  false,  1,  false,   6,    MPLS_06_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_06_Exp            "},
{  false,  1,  false,   6,    MPLS_06_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_06_Stack          "},
{  false,  0,  false,   6,    MPLS_06_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_06_Ttl            "},
{  false,  2,  false,   7,    MPLS_07_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_07_Label          "},
{  false,  1,  false,   7,    MPLS_07_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_07_Exp            "},
{  false,  1,  false,   7,    MPLS_07_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_07_Stack          "},
{  false,  0,  false,   7,    MPLS_07_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_07_Ttl            "},
{  false,  2,  false,   8,    MPLS_08_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_08_Label          "},
{  false,  1,  false,   8,    MPLS_08_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_08_Exp            "},
{  false,  1,  false,   8,    MPLS_08_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_08_Stack          "},
{  false,  0,  false,   8,    MPLS_08_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_08_Ttl            "},
{  false,  0,  false,   1,    VLAN_01_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_01_Tpi            "},
{  false,  2,  false,   1,    VLAN_01_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_01_Tci_Pcp        "},
{  false,  1,  false,   1,    VLAN_01_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_01_Tci_Cfi        "},
{  false,  1,  false,   1,    VLAN_01_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_01_Vid            "},
{  false,  0,  false,   2,    VLAN_02_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_02_Tpi            "},
{  false,  2,  false,   2,    VLAN_02_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_02_Tci_Pcp        "},
{  false,  1,  false,   2,    VLAN_02_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_02_Tci_Cfi        "},
{  false,  1,  false,   2,    VLAN_02_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_02_Vid            "},
{  false,  0,  false,   3,    VLAN_03_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_03_Tpi            "},
{  false,  2,  false,   3,    VLAN_03_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_03_Tci_Pcp        "},
{  false,  1,  false,   3,    VLAN_03_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_03_Tci_Cfi        "},
{  false,  1,  false,   3,    VLAN_03_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_03_Vid            "},
{  false,  0,  false,   4,    VLAN_04_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_04_Tpi            "},
{  false,  2,  false,   4,    VLAN_04_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_04_Tci_Pcp        "},
{  false,  1,  false,   4,    VLAN_04_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_04_Tci_Cfi        "},
{  false,  1,  false,   4,    VLAN_04_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_04_Vid            "},
{  false,  0,  false,   5,    VLAN_05_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_05_Tpi            "},
{  false,  2,  false,   5,    VLAN_05_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_05_Tci_Pcp        "},
{  false,  1,  false,   5,    VLAN_05_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_05_Tci_Cfi        "},
{  false,  1,  false,   5,    VLAN_05_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_05_Vid            "},
{  false,  0,  false,   6,    VLAN_06_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_06_Tpi            "},
{  false,  2,  false,   6,    VLAN_06_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_06_Tci_Pcp        "},
{  false,  1,  false,   6,    VLAN_06_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_06_Tci_Cfi        "},
{  false,  1,  false,   6,    VLAN_06_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_06_Vid            "},
{  false,  0,  false,   7,    VLAN_07_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_07_Tpi            "},
{  false,  2,  false,   7,    VLAN_07_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_07_Tci_Pcp        "},
{  false,  1,  false,   7,    VLAN_07_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_07_Tci_Cfi        "},
{  false,  1,  false,   7,    VLAN_07_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_07_Vid            "},
{  false,  0,  false,   8,    VLAN_08_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_08_Tpi            "},
{  false,  2,  false,   8,    VLAN_08_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_08_Tci_Pcp        "},
{  false,  1,  false,   8,    VLAN_08_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_08_Tci_Cfi        "},
{  false,  1,  false,   8,    VLAN_08_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_08_Vid            "},
{  false,  0,  false,   0,    Num_VLAN_Tags            ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "Num_VLAN_Tags          "},
{  false,  0,  false,   0,    Num_MPLS_Hdrs            ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "Num_MPLS_Hdrs          "},
{  false,  0,  false,   0,    STREAM_Type              ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Type            "},
{  false,  0,  false,   0,    STREAM_Pkts_Per_Burst    ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Pkts_Per_Burst  "},
{  false,  0,  false,   0,    STREAM_Burst_Per_Stream  ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Burst_Per_Stream"},
{  false,  0,  false,   0,    STREAM_Inter_Burst_Gap   ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Inter_Burst_Gap "},
{  false,  0,  false,   0,    STREAM_Inter_Stream_Gap  ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Inter_Stream_Gap"},
{  false,  0,  false,   0,    STREAM_Start_Delay       ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Start_Delay     "},
{  false,  0,  false,   0,    STREAM_Rate_Type         ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Rate_Type       "},
{  false,  0,  false,   0,    STREAM_Rate              ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Rate            "},
{  false,  0,  false,   0,    STREAM_Ipg               ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Ipg             "},
{  false,  0,  false,   0,    STREAM_Percentage        ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Percentage      "},
{  false,  0,  false,   0,    STREAM_Pkts_Per_Sec      ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Pkts_Per_Sec    "},
{  false,  0,  false,   0,    STREAM_Bit_Rate          ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Bit_Rate        "},
{  false,  0,  false,   0,    STREAM_Crc_Enable        ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Crc_Enable      "},
{  false,  0,  false,   0,    STREAM_Timestamp_Enable  ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Timestamp_Enable"},
{  false,  0,  false,   0,    MAC_Control              ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "MAC_Control            "},
{  false,  0,  false,   0,    MAC_Control_Opcode       ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "MAC_Control_Opcode     "},
{  false,  0,  false,   0,    Pause_Quanta             ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta           "},
{  false,  0,  false,   0,    Priority_En_Vector       ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Priority_En_Vector     "},
{  false,  0,  false,   0,    Pause_Quanta_0           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_0         "},
{  false,  0,  false,   0,    Pause_Quanta_1           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_1         "},
{  false,  0,  false,   0,    Pause_Quanta_2           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_2         "},
{  false,  0,  false,   0,    Pause_Quanta_3           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_3         "},
{  false,  0,  false,   0,    Pause_Quanta_4           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_4         "},
{  false,  0,  false,   0,    Pause_Quanta_5           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_5         "},
{  false,  0,  false,   0,    Pause_Quanta_6           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_6         "},
{  false,  0,  false,   0,    Pause_Quanta_7           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_7         "},
{  false,  0,  false,   0,    Zeros_8Bit               ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "Zeros_8Bit             "},
{  false,  0,  false,   0,    TCP_Total_Len            ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Total_Len          "}
};

// header to fields map
map <cea_hdr_type, vector <cea_field_id>> htof = {
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
               }}
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
        logfile.open("run.log", ofstream::out);
        if (!logfile.is_open()) {

            exit(1);
        }
    }
    ~cea_init() { logfile.close(); }
};
cea_init init;

//------------------------------------------------------------------------------
// support for PCAP write
//------------------------------------------------------------------------------

// pcap global header
struct CEA_PACKED pcap_file_hdr {
    uint32_t magic         : 32;
    uint16_t version_major : 16;
    uint16_t version_minor : 16;
    uint32_t thiszone      : 32;
    uint32_t sigfigs       : 32;
    uint32_t snaplen       : 32;
    uint32_t linktype      : 32;
};

// pcap per frame header
struct CEA_PACKED pcap_frame_hdr {
    uint32_t tv_sec  : 32;
    uint32_t tv_usec : 32;
    uint32_t caplen  : 32;
    uint32_t len     : 32;
};

// true if the file exists, else false
bool file_exists(const string& filename) {
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1) {
        return true;
    }
    return false;
}

void write_pcap(unsigned char *frame, uint32_t len) {
    pcap_file_hdr fh;
    pcap_frame_hdr ph;

    fh.magic= 0xa1b2c3d4;
    fh.version_major = 2;  
    fh.version_minor = 4;  
    fh.thiszone = 0;       
    fh.sigfigs = 0;        
    fh.snaplen = 4194304;
    fh.linktype = 1;     

    ph.tv_sec  = 0;
    ph.tv_usec = 0;
    ph.caplen = len;
    ph.len = len;

    unsigned char buf[CEA_MAX_FRAME_SIZE];
    uint32_t offset = 0;
    memset(buf, 0, CEA_MAX_FRAME_SIZE);

    ofstream pcapfile;

    // TODO: what if the run.pcap was present from a previous run
    // the file will exist and hence the pcapfile handle will not be created
    // make sure the old file is deleted
    // move the file_exists check to a constructor of the stream
    if (!file_exists("run.pcap")) {
        pcapfile.open("run.pcap", ofstream::app);
        pcapfile.write((char*)&fh, sizeof(pcap_file_hdr));
    } else {
        pcapfile.open("run.pcap", ofstream::app);
    }

    memcpy(buf+offset, (char*)&ph, sizeof(pcap_frame_hdr));
    offset += sizeof(pcap_frame_hdr);
    memcpy(buf+offset, frame, len);
    offset += len;

    pcapfile.write((const char*)buf, offset);
    pcapfile.close();
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

// calculate and return checksum in network byte order
uint16_t compute_ipv4_csum(unsigned char *vdata,size_t length) {
    // Cast the data pointer to one that can be indexed.
    // char* data=(char*)vdata;
    unsigned char *data=vdata;

    // Initialise the accumulator.
    uint64_t acc=0xffff;

    // Handle any partial block at the start of the data.
    unsigned int offset=((uintptr_t)data)&3;
    if (offset) {
        size_t count=4-offset;
        if (count>length) count=length;
        uint32_t word=0;
        memcpy(offset+(char*)&word,data,count);
        acc+=ntohl(word);
        data+=count;
        length-=count;
    }

    // Handle any complete 32-bit blocks.
    unsigned char* data_end=data+(length&~3);
    while (data!=data_end) {
        uint32_t word;
        memcpy(&word,data,4);
        acc+=ntohl(word);
        data+=4;
    }
    length&=3;

    // Handle any partial block at the end of the data.
    if (length) {
        uint32_t word=0;
        memcpy(&word,data,length);
        acc+=ntohl(word);
    }

    // Handle deferred carries.
    acc=(acc&0xffffffff)+(acc>>32);
    while (acc>>16) {
        acc=(acc&0xffff)+(acc>>16);
    }

    // If the data began at an odd byte address
    // then reverse the byte order to compensate.
    if (offset&1) {
        acc=((acc&0xff00)>>8)|((acc&0x00ff)<<8);
    }

    // Return the checksum in network byte order.
    return htons(~acc);
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

#define CEA_FORMATTED_HDR_LEN 80
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
    const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int divider = type;

    while (size > (divider-1)) {
      size /= divider;
      i++;
    }
    buf << fixed << setprecision(2) << size << " " << units[i] << endl; 
    return buf.str();
}

// stringize cea_frame_type
string to_str(cea_frame_type t) {
    string name;
    switch(t) {
        case ETH_V2   : { name = "ETH_V2  "; break; }
        case ETH_LLC  : { name = "ETH_LLC "; break; }
        case ETH_SNAP : { name = "ETH_SNAP"; break; }
        default       : { name = "undefined"; break; }
    }
    return cea_trim(name);
}

// stringize cea_hdr_type
string to_str(cea_hdr_type t) {
    string name;
    switch(t) {
        case MAC      : { name = "MAC     "; break; }
        case LLC      : { name = "LLC     "; break; }
        case SNAP     : { name = "SNAP    "; break; }
        case IPv4     : { name = "IPv4    "; break; }
        case IPv6     : { name = "IPv6    "; break; }
        case ARP      : { name = "ARP     "; break; }
        case TCP      : { name = "TCP     "; break; }
        case UDP      : { name = "UDP     "; break; }
        case PAUSE    : { name = "PAUSE   "; break; }
        case PFC      : { name = "PFC     "; break; }
        case UDP_PHDR : { name = "UDP_PHDR"; break; }
        case TCP_PHDR : { name = "TCP_PHDR"; break; }
        default       : { name = "undefined"; break; }
    }
    return cea_trim(name);
}

// stringize cea_msg_verbosity
string to_str(cea_msg_verbosity t) {
    string name;
    switch(t) {
        case LOW  : { name = "LOW  "; break; }
        case FULL : { name = "FULL "; break; }
        default   : { name = "UNSET"; break; }
    }
    return cea_trim(name);
}

// stringize cea_field_generation_type
string to_str(cea_field_generation_type t) {
    string name;
    switch(t) {
        case Fixed             : { name = "Fixed            "; break; }                                    
        case Random            : { name = "Random           "; break; }                                    
        case Random_in_Range   : { name = "Random_in_Range  "; break; }                                    
        case Increment         : { name = "Increment        "; break; }                                    
        case Decrement         : { name = "Decrement        "; break; }                                    
        case Increment_Cycle   : { name = "Increment_Cycle  "; break; }                                    
        case Decrement_Cycle   : { name = "Decrement_Cycle  "; break; }                                   
        case Incr_Byte         : { name = "Incr_Byte        "; break; }                                    
        case Incr_Word         : { name = "Incr_Word        "; break; }                                      
        case Decr_Byte         : { name = "Decr_Byte        "; break; }                                       
        case Decr_Word         : { name = "Decr_Word        "; break; }                                        
        case Repeat_Pattern    : { name = "Repeat_Pattern   "; break; }                                         
        case Fixed_Pattern     : { name = "Fixed_Pattern    "; break; }                                          
        case Continuous_Pkts   : { name = "Continuous_Pkts  "; break; }                                  
        case Continuous_Burst  : { name = "Continuous_Burst "; break; }                                   
        case Stop_After_Stream : { name = "Stop_After_Stream"; break; }                                    
        case Goto_Next_Stream  : { name = "Goto_Next_Stream "; break; }                                   
        case Ipg               : { name = "Ipg              "; break; }                      
        case Percentage        : { name = "Percentage       "; break; }                             
        case Pkts_Per_Sec      : { name = "Pkts_Per_Sec     "; break; }                               
        case Bit_Rate          : { name = "Bit_Rate         "; break; }                           
        default                : { name = "undefined        "; break; }
    }
    return cea_trim(name);
}

// global variable to track proxy and stream id
uint32_t proxy_id = 0;
uint32_t stream_id = 0;

//------------------------------------------------------------------------------
// Timer class for performance measurement
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

//------------------------------------------------------------------------------
// Stream implementation
//------------------------------------------------------------------------------

class cea_stream::core {
public:    
    // constructor
    core(string name);

    // fucntion to set the field to a fixed value
    void set(cea_field_id id, uint64_t value);

    // function to assign a field to an inbuilt value generator
    // with default specifications
    void set(cea_field_id id, cea_field_generation_type spec);

    // function to assign a field to an inbuilt value generator
    // with custom specifications
    void set(cea_field_id id, cea_field_generation_type mspec,
        cea_field_generation_spec vspec);

    // set when the proxy object is created
    string stream_name;

    // automatically assigned when the proxy object is created
    // the value of the field is set from the global variable proxy_id
    uint32_t stream_id;

    // (container2) a list of all the field ids (in sequence) of the 
    // selected frame type and headers output of arrange_fields_in_sequence
    vector<uint32_t> fseq;

    // (container3) a sequence of all the fields ids that needs 
    // generation of values output of purge_static_fields
    vector<uint32_t> cseq;

    // (container4) field matrix
    vector<cea_field> fields;

    // based on user specification of the frame, build a vector of 
    // field ids in the sequence required by the specification
    void arrange_fields_in_sequence();

    // build cseq by parsing fseq and adding only mutable fields
    void purge_static_fields();

    // calls arrange and purge
    void prune();

    // build the principal frame using fseq
    void build_base_frame();

    // concatenate all fields reuired by the frame spec
    uint32_t splice_fields(vector<uint32_t> seq, unsigned char *buf);

    // base frame buffer
    unsigned char *base_frame;

    // total or initial len of the principal frame
    uint32_t base_frame_len;

    // (debug only) print all the bytes of the principal frame in hex
    void print_base_frame();

    // reset stream to factory settings
    void reset();

    // overload =
    void do_copy(const cea_stream *rhs);

    // overload <<
    string describe() const;

    // calculate offset in fseq
    void build_offsets();

    // get offset of a particular field by parsing fseq
    uint32_t get_offset(cea_field_id id);

    // get length (in bits) of a field from fields
    uint32_t get_len(cea_field_id id);

    // get current value of a field from fields
    uint64_t get_value(cea_field_id id);

    // build a string to be prefixed in all messages generated from this class
    string msg_prefix;

    // calculate udp checksum along with the conceptual pseudoheader
    uint32_t compute_udp_csum();

    // calculate tcp checksum along with the conceptual pseudoheader
    uint32_t compute_tcp_csum();

    // if at any point of time a char array is required, scratchpad can be used
    // it is created during stream constructor, it should never be freed 
    unsigned char *scratchpad;

    // slow crc method
    uint32_t compute_crc32(unsigned char *data, uint32_t len);

    // print functions
    void print_stream_properties();
    void print_base_frame_properties();

    // cache variables

    // length of payload after removing headers and preamble
    uint32_t payload_len;

    // offset of the payload from preamble to the last field of the last header 
    uint32_t payload_offset;

    uint32_t crc_len;
};

cea_stream::~cea_stream() = default;

void cea_stream::core::print_base_frame_properties() {
    cealog << endl << cea_formatted_hdr("Base Frame Properties");
    cealog << setw(20) << left << "Frame len: " 
        << get_value(FRAME_Len) << endl;

    cealog << setw(20) << left << "Preamble len: "
        << get_len(MAC_Preamble) << endl;

    cealog << setw(20) << left << "Headers len: " 
        << base_frame_len - get_len(MAC_Preamble) << endl;

    cealog << setw(20) << left << "Payload len: "
        << payload_len << endl;

    cealog << setw(20) << left << "Payload offset: "
        << payload_offset << endl;
}

void cea_stream::core::print_stream_properties() {
    cealog << endl << cea_formatted_hdr("Stream Properties");
    for (uint32_t idx=STREAM_Type; idx<=STREAM_Timestamp_Enable; idx++) {
        cealog << left
            << cea_trim(fields[idx].name)
            << string((30 - cea_trim(fields[idx].name).length()), '.') 
            << " " << fields[idx].value << " "
            << ((fields[idx].touched)? "(user)" : "")
            << endl;
    }
}

uint32_t cea_stream::core::compute_crc32(unsigned char *data, uint32_t len) {
    uint32_t crc;
    crc = ~0U;
    while (len--)
    	crc = crc32_tab[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
    return crc ^ ~0U;
}

void cea_stream::core::prune() {
    arrange_fields_in_sequence();
    purge_static_fields();
}

uint32_t cea_stream::core::get_offset(cea_field_id id) {
    uint32_t total_bits = 0;
    for (auto i: fseq) {
        if (fields[i].id == id)
            break;
        total_bits += fields[i].len;
    }
    return (total_bits/8);
}

uint32_t cea_stream::core::get_len(cea_field_id id) {
    return (fields[id].len/8);
}

uint64_t cea_stream::core::get_value(cea_field_id id) {
    return (fields[id].value);
}

void cea_stream::core::build_offsets() {
    uint32_t cntr = 0;
    for (auto i: fseq) {
        if (cntr==0) {
            fields[i].offset = 0;
        } else {
            auto prev_offset = fseq.begin() + (cntr-1);
            fields[i].offset = floor((double)fields[*prev_offset].len/8) + 
                fields[*prev_offset].offset;
        }
        cntr++;
    }
}

void cea_stream::core::print_base_frame() {
    ostringstream buf("");
    buf.setf(ios::hex, ios::basefield);
    buf.setf(ios_base::left);
    buf << endl;
    buf << cea_formatted_hdr("Base Frame");
    
    for (uint32_t idx=0; idx<(get_value(FRAME_Len) + get_len(MAC_Preamble)); 
            idx++) {
        buf << setw(2) << right << setfill('0')<< hex << 
            (uint16_t) base_frame[idx] << " ";
        if (idx%8==7) buf << " ";
        if (idx%16==15) buf  << "(" << dec << (idx+1) << ")" << endl;
    }
    buf << endl << endl;

    cealog << buf.str();
}

// algorithm to arrange the frame fields
void cea_stream::core::arrange_fields_in_sequence() {
    
    fseq.push_back(MAC_Preamble);

    fseq.insert(fseq.end(),
        htof[MAC].begin(),
        htof[MAC].end());

    for (uint32_t idx=VLAN_01_Tpi; idx<=VLAN_08_Vid; idx++) {
        if (fields[idx].added) {
            fseq.push_back(idx);
        }
    }

    if (fields[FRAME_Type].value == ETH_V2) {
        fseq.push_back(MAC_Ether_Type);
    } else {
        fseq.push_back(MAC_Len);
    }

    if (fields[FRAME_Type].value == ETH_LLC) {
        fseq.insert(fseq.end(), htof[LLC].begin(), htof[LLC].end());
    }

    if (fields[FRAME_Type].value == ETH_SNAP) {
        fseq.insert(fseq.end(), htof[LLC].begin(), htof[LLC].end());
        fseq.insert(fseq.end(), htof[SNAP].begin(), htof[SNAP].end());
    }

    for (uint32_t idx=MPLS_01_Label; idx<=MPLS_08_Ttl; idx++) {
        if (fields[idx].added) {
            fseq.push_back(idx);
        }
    }

    // ARP does not contain IP or TCP/UDP headers
    if (fields[Network_Hdr].value == ARP) {
        fseq.insert(fseq.end(), 
            htof[(cea_hdr_type)fields[Network_Hdr].value].begin(), 
            htof[(cea_hdr_type)fields[Network_Hdr].value].end());
    } else {
        fseq.insert(fseq.end(), 
            htof[(cea_hdr_type)fields[Network_Hdr].value].begin(), 
            htof[(cea_hdr_type)fields[Network_Hdr].value].end());
    
        if (get_value(Transport_Hdr)==UDP)
            set(IPv4_Protocol, 17);
        else if (get_value(Transport_Hdr)==UDP)
            set(IPv4_Protocol, 6);

        fseq.insert(fseq.end(), 
            htof[(cea_hdr_type)fields[Transport_Hdr].value].begin(), 
            htof[(cea_hdr_type)fields[Transport_Hdr].value].end());
    }
}

void cea_stream::core::purge_static_fields() {
    for (auto i: fseq) {
        if (fields[i].touched) {
            cseq.push_back(i);
        }
    }
}

uint32_t cea_stream::core::compute_udp_csum() {
    vector<uint32_t>nseq;
    for (auto i: htof[UDP_PHDR]) {
        nseq.push_back(i);
    }
    uint32_t offset = splice_fields(nseq, scratchpad);

    memcpy(scratchpad+offset, base_frame+get_offset(UDP_Src_Port), 
        get_value(UDP_Len));
    return (compute_ipv4_csum(scratchpad, (offset+get_value(UDP_Len))));
}

uint32_t cea_stream::core::compute_tcp_csum() {
    vector<uint32_t>nseq;
    for (auto i: htof[TCP_PHDR]) {
        nseq.push_back(i);
    }
    uint32_t offset = splice_fields(nseq, scratchpad);

    // copy tcp header and payload
    memcpy(scratchpad+offset, base_frame+get_offset(TCP_Src_Port), 
        get_value(TCP_Total_Len));
    return (compute_ipv4_csum(scratchpad, (offset+get_value(TCP_Total_Len))));
}

void print_cdata (unsigned char* tmp, int len) {
    stringstream s;
    s.str("");
    uint32_t idx = 0;

    for (int x=0; x<len/16; x++) {
        for (int y=0; y<16; y++) {
            s <<noshowbase<<setw(2)<<setfill('0')<<hex<<uint16_t(tmp[idx])<<" ";
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

// concatenate all fields reuired by the frame spec
uint32_t cea_stream::core::splice_fields(vector<uint32_t> seq, 
        unsigned char *buf) {
    uint32_t offset = 0;
    uint64_t merged = 0;
    uint64_t len = 0;
    uint64_t mlen = 0;

    for (uint32_t i=0; i<seq.size(); i++) {
        uint32_t idx = fields[seq[i]].id;
        if (fields[idx].merge != 0) {
            merged = fields[idx].value; // first field
            mlen += fields[idx].len;
            for (uint32_t x=(i+1); x<=((i+fields[idx].merge)); x++) {
                uint32_t xidx = fields[seq[x]].id;
                len = fields[xidx].len;
                merged = (merged << len) | fields[xidx].value;
                mlen += len;
            }
            cea_memcpy_ntw_byte_order(buf+offset, (char*)&merged, mlen/8);
            offset += mlen/8;
            i += fields[idx].merge; // skip mergable entries
        } else {
            uint64_t tmp = fields[idx].value;
            uint64_t len = fields[idx].len;
            cea_memcpy_ntw_byte_order(buf+offset, (char*)&tmp, len/8);
            offset += len/8;
            mlen = 0; // TODO fix this
            merged = 0; // TODO fix this
        }
    }
    return offset;
}

void cea_stream::core::build_base_frame() {
    // total length of all headers (in bits) includes preamble 
    // but minus payload
    for (auto i : fseq) {
        base_frame_len += fields[i].len;
    }
    base_frame_len /= 8; // in bytes

    crc_len = (get_value(STREAM_Crc_Enable)? 4:0);

    // throw error if base_frame_len is greater than user specified FRAME_Len 
    // TODO: consider CRC also
    if ((base_frame_len - get_len(MAC_Preamble)) > get_value(FRAME_Len)) {
        CEA_MSG("*** ERROR: "
            << "Final frame lenght is greater then the length "
            << "specified via FRAME_Len. "
            << "Final Frame Length: " << base_frame_len << "  "
            << "Desired Frame Length: " << get_value(FRAME_Len)
            )
        exit(0);
    }
    payload_len = get_value(FRAME_Len) -
        (base_frame_len - get_len(MAC_Preamble));

    payload_offset = base_frame_len;
        
    // compute length field in IPv4 header
    uint32_t iplen = get_value(FRAME_Len) - get_offset(IPv4_Version) +
        get_len(MAC_Preamble) - crc_len;
    set(IPv4_Total_Len, iplen);

    // compute length field in UDP header
    uint32_t udplen = get_value(FRAME_Len) - get_offset(UDP_Src_Port) +
        get_len(MAC_Preamble) - crc_len;
    set(UDP_Len, udplen);

    set(TCP_Total_Len, ((get_value(TCP_Data_Offset)*4) + payload_len - crc_len));

    // build the frame
    splice_fields(fseq, base_frame);
    
    // fill dummy payload value
    for (uint32_t i=0; i<payload_len; i++) {
        memcpy(base_frame+(payload_offset+i), (char*)&i, 1);
    }
    // find ipv4 csum and overlay on the base frame
    if (get_value(Network_Hdr) == IPv4) {
        uint16_t ip_csum = compute_ipv4_csum(
            base_frame+get_offset(IPv4_Version), 20);

        memcpy(base_frame+get_offset(IPv4_Hdr_Csum), (char*)&ip_csum, 2);
    }
    // find udp csum and overlay on the base frame
    if (get_value(Transport_Hdr) == UDP) {
        uint16_t udp_csum = compute_udp_csum();
        memcpy(base_frame+get_offset(UDP_Csum), (char*)&udp_csum, 2);
    }
    // find tcp csum and overlay on the base frame
    if (get_value(Transport_Hdr) == TCP) {
        uint16_t tcp_csum = compute_tcp_csum();
        memcpy(base_frame+get_offset(TCP_Csum), (char*)&tcp_csum, 2);
    }
    // insert CRC32
    if (get_value(STREAM_Crc_Enable)) {
        uint32_t crc = compute_crc32(base_frame+get_len(MAC_Preamble), 
            (get_value(FRAME_Len)-crc_len));

        memcpy(base_frame+(get_value(FRAME_Len)+get_len(MAC_Preamble)-crc_len),
            (char*)&crc, crc_len);
    }
    #ifdef CEA_DEBUG
    print_stream_properties();
    print_base_frame_properties();
    print_base_frame();
    write_pcap((base_frame + get_len(MAC_Preamble)), get_value(FRAME_Len));
    #endif
}

// constructor
cea_stream::core::core(string name) {
    stream_id = cea::stream_id;
    cea::stream_id++;
    stream_name = name + ":" + to_string(stream_id);
    reset();
}

cea_stream::cea_stream(string name) : impl (new core(name)) {
}

// copy constructor
cea_stream::cea_stream(const cea_stream &rhs) {
    // impl->do_copy(&rhs); // TODO
}

// assign operator overload
cea_stream& cea_stream::operator = (cea_stream &rhs) {
   if (this != &rhs) {
      impl->do_copy(&rhs);
   }
   return *this;
}

// print operator overload
ostream& operator << (ostream &os, const cea_stream &f) {
    os << f.impl->describe();
    return os;
}

// utility
bool in_range(uint32_t low, uint32_t high, uint32_t x) {        
    return (low <= x && x <= high);         
}

void cea_stream::set(cea_field_id id, uint64_t value) {
    impl->set(id, value);
}

// user api to set fields of the stream
void cea_stream::core::set(cea_field_id id, uint64_t value) {
    // process if mpls
    bool mpls = in_range(MPLS_01_Label, MPLS_08_Ttl, id);
    if (mpls) {
        fields[Num_MPLS_Hdrs].value += 1;
        uint32_t stack = fields[id].stack;
        // mark all fields of this stack
        for (uint32_t idx=MPLS_01_Label; idx<=MPLS_08_Ttl; idx++) {
            if (fields[idx].stack == stack) {
                fields[idx].added = true;
            }
        }
    }
    // process if vlan
    bool vlan = in_range(VLAN_01_Tpi, VLAN_08_Vid, id);
    if (vlan) {
        fields[Num_VLAN_Tags].value += 1;
        uint32_t stack = fields[id].stack;
        // touch all fields of this stack
        for (uint32_t idx=VLAN_01_Tpi; idx<=VLAN_08_Vid; idx++) {
            if (fields[idx].stack == stack)
                fields[idx].added = true;
        }
    }
    fields[id].value = value;
    fields[id].touched = true;
}

void cea_stream::set(cea_field_id id, cea_field_generation_type spec) {
    impl->set(id, spec);
}

// user api to set fields of the stream
void cea_stream::core::set(cea_field_id id, cea_field_generation_type spec) {
    // process if mpls
    bool mpls = in_range(MPLS_01_Label, MPLS_08_Ttl, id);
    if (mpls) {
        fields[Num_MPLS_Hdrs].value += 1;
        uint32_t stack = fields[id].stack;
        // touch all fields of this stack
        for (uint32_t idx=MPLS_01_Label; idx<=MPLS_08_Ttl; idx++) {
            if (fields[idx].stack == stack)
                fields[idx].added = true;
        }
    }
    // process if vlan
    bool vlan = in_range(VLAN_01_Tpi, VLAN_08_Vid, id);
    if (vlan) {
        fields[Num_VLAN_Tags].value += 1;
        uint32_t stack = fields[id].stack;
        // touch all fields of this stack
        for (uint32_t idx=VLAN_01_Tpi; idx<=VLAN_08_Vid; idx++) {
            if (fields[idx].stack == stack)
                fields[idx].added = true;
        }
    }
    fields[id].touched = true;
    fields[id].gen_type = spec;
}

void cea_stream::set(cea_field_id id, cea_field_generation_type mspec,
        cea_field_generation_spec vspec) {
    impl->set(id, mspec, vspec);
}

void cea_stream::core::set(cea_field_id id, cea_field_generation_type mspec,
        cea_field_generation_spec vspec) {
    // process if mpls
    bool mpls = in_range(MPLS_01_Label, MPLS_08_Ttl, id);
    if (mpls) {
        fields[Num_MPLS_Hdrs].value += 1;
        uint32_t stack = fields[id].stack;
        // touch all fields of this stack
        for (uint32_t idx=MPLS_01_Label; idx<=MPLS_08_Ttl; idx++) {
            if (fields[idx].stack == stack)
                fields[idx].added = true;
        }
    }
    // process if vlan
    bool vlan = in_range(VLAN_01_Tpi, VLAN_08_Vid, id);
    if (vlan) {
        fields[Num_VLAN_Tags].value += 1;
        uint32_t stack = fields[id].stack;
        // touch all fields of this stack
        for (uint32_t idx=VLAN_01_Tpi; idx<=VLAN_08_Vid; idx++) {
            if (fields[idx].stack == stack)
                fields[idx].added = true;
        }
    }
    fields[id].touched = true;
    fields[id].gen_type = mspec;
    fields[id].value = vspec.value;
    fields[id].start = vspec.range_start;
    fields[id].stop = vspec.range_stop;
    fields[id].step = vspec.range_step;
    fields[id].repeat = vspec.repeat_after;
}

void cea_stream::core::do_copy(const cea_stream *rhs) {
}

void cea_stream::core::reset() {
    if (!base_frame) {
        CEA_DBG("INFO: base_frame is not null, delete existing buffer");
        delete(base_frame);
    }
    base_frame = new unsigned char[CEA_MAX_FRAME_SIZE];
    memset(base_frame, 0, CEA_MAX_FRAME_SIZE);

    if (!scratchpad) {
        CEA_DBG("INFO: scratchpad is not null, delete existing buffer");
        delete(scratchpad);
    }
    scratchpad = new unsigned char[CEA_SCRATCHPAD_SIZE];
    memset(scratchpad, 0, CEA_SCRATCHPAD_SIZE);

    base_frame_len = 0;
    payload_len = 0;
    payload_offset = 0;
    crc_len = 0;
    fields = flds;
    msg_prefix = '(' + stream_name + ") ";
}

#define CEA_FLDWIDTH 8
string cea_stream::core::describe() const {
    ostringstream buf("");
    buf.setf(ios::hex, ios::basefield);
    buf.setf(ios_base::left);
    buf << cea_formatted_hdr("Stream Definition");

    buf << setw(CEA_FLDWIDTH)   << "Touch" 
        << setw(CEA_FLDWIDTH)   << "Merge"  
        << setw(CEA_FLDWIDTH)   << "Mask "  
        << setw(CEA_FLDWIDTH)   << "Id   "  
        << setw(CEA_FLDWIDTH)   << "Len  "  
        << setw(CEA_FLDWIDTH)   << "Offset"  
        << setw(CEA_FLDWIDTH+2)   << "Modifier" 
        << setw(CEA_FLDWIDTH+10) << "Value"  
        << setw(CEA_FLDWIDTH)   << "Start"  
        << setw(CEA_FLDWIDTH)   << "Stop "  
        << setw(CEA_FLDWIDTH)   << "Step "  
        << setw(CEA_FLDWIDTH)   << "Repeat"
        << "Name" << endl;

    for (uint32_t id = 0; id <cea::Network_Hdr; id++) {
        buf << setw(CEA_FLDWIDTH) << fields[id].touched 
            << setw(CEA_FLDWIDTH) << fields[id].merge    
            << setw(CEA_FLDWIDTH) << fields[id].added
            << setw(CEA_FLDWIDTH) << fields[id].id       
            << setw(CEA_FLDWIDTH) << fields[id].len      
            << setw(CEA_FLDWIDTH) << fields[id].offset    
            << setw(CEA_FLDWIDTH+2) << fields[id].gen_type 
            << setw(CEA_FLDWIDTH+10) << to_str((cea_frame_type) fields[id].value)
            << setw(CEA_FLDWIDTH) << fields[id].start    
            << setw(CEA_FLDWIDTH) << fields[id].stop     
            << setw(CEA_FLDWIDTH) << fields[id].step     
            << setw(CEA_FLDWIDTH) << fields[id].repeat
            << fields[id].name;
            buf << endl;
    }
    for (uint32_t id = cea::Network_Hdr; id <cea::VLAN_Tag; id++) {
        buf << setw(CEA_FLDWIDTH) << fields[id].touched 
            << setw(CEA_FLDWIDTH) << fields[id].merge    
            << setw(CEA_FLDWIDTH) << fields[id].added
            << setw(CEA_FLDWIDTH) << fields[id].id       
            << setw(CEA_FLDWIDTH) << fields[id].len      
            << setw(CEA_FLDWIDTH) << fields[id].offset    
            << setw(CEA_FLDWIDTH+2) << fields[id].gen_type 
            << setw(CEA_FLDWIDTH+10) << to_str((cea_hdr_type) fields[id].value)
            << setw(CEA_FLDWIDTH) << fields[id].start    
            << setw(CEA_FLDWIDTH) << fields[id].stop     
            << setw(CEA_FLDWIDTH) << fields[id].step     
            << setw(CEA_FLDWIDTH) << fields[id].repeat
            << fields[id].name;
            buf << endl;
    }
    for (uint32_t id = VLAN_Tag; id<cea::Num_Fields; id++) {
        buf << setw(CEA_FLDWIDTH) << fields[id].touched 
            << setw(CEA_FLDWIDTH) << fields[id].merge    
            << setw(CEA_FLDWIDTH) << fields[id].added     
            << setw(CEA_FLDWIDTH) << fields[id].id       
            << setw(CEA_FLDWIDTH) << fields[id].len      
            << setw(CEA_FLDWIDTH) << fields[id].offset    
            << setw(CEA_FLDWIDTH+2) << fields[id].gen_type 
            << setw(CEA_FLDWIDTH+10) << fields[id].value    
            << setw(CEA_FLDWIDTH) << fields[id].start    
            << setw(CEA_FLDWIDTH) << fields[id].stop     
            << setw(CEA_FLDWIDTH) << fields[id].step     
            << setw(CEA_FLDWIDTH) << fields[id].repeat
            << fields[id].name;
            buf << endl;
    }
    return buf.str();
}

//------------------------------------------------------------------------------
// Proxy Implementation
//------------------------------------------------------------------------------

class cea_proxy::core {
public:
    // set when the proxy object is created
    string proxy_name;

    core(string name);

    // add stream to proxy queue
    void add_stream(cea_stream *stm);

    // add a command (in the form of cea_stream) to proxy queue
    void add_cmd(cea_stream *stm);

    // execute a command immediately does not add to proxy queue
    void exec_cmd(cea_stream *stm);

    void start();

    // automatically assigned when the proxy object is created
    // the value of the field is set from the global variable proxy_id
    uint32_t proxy_id;

    // user's test streams will be pushed into this queue (container1)
    vector<cea_stream*> stmq;

    // handle to the stream being processed
    cea_stream *cur_stm;

    // main thread
    thread worker_tid;
    void worker();
    void start_worker();
    void join_threads();

    // worker modules
    void read_next_stream_from_stmq();
    void extract_traffic_parameters();
    void begin_mutation();

    // buffer to store the generated frames
    void *fbuf;
    void create_frame_buffer();
    void release_frame_buffer();

    void reset();
    string msg_prefix;

    cea_timer timer;
};

cea_proxy::~cea_proxy() = default;

cea_proxy::cea_proxy(string name) : impl(new core(name)){
}

void cea_proxy::add_stream(cea_stream *stm) {
    impl->stmq.push_back(stm);
}

void cea_proxy::add_cmd(cea_stream *stm) {
    impl->stmq.push_back(stm);
}

void cea_proxy::exec_cmd(cea_stream *stm) {
    impl->exec_cmd(stm);
}

// constructor
cea_proxy::core::core(string name) {
    proxy_id = cea::proxy_id;
    cea::proxy_id++;
    proxy_name = name + ":" + to_string(proxy_id);
    reset();
    CEA_MSG("Proxy created with name=" << name << " and id=" << proxy_id);
}

void cea_proxy::core::add_stream(cea_stream *stm) {
    stmq.push_back(stm);
}

void cea_proxy::core::add_cmd(cea_stream *stm) {
    stmq.push_back(stm);
}

void cea_proxy::core::exec_cmd(cea_stream *stm) {
}

void cea_proxy::core::reset() {
    msg_prefix = '(' + proxy_name + ") ";
}

void cea_proxy::start() {
    impl->start();
}

// start stream processing and generate frames
void cea_proxy::core::start() {
    start_worker();
    join_threads();
}

void cea_proxy::core::join_threads() {
    worker_tid.join();
}

void cea_proxy::core::start_worker() {
    worker_tid = thread(&cea_proxy::core::worker, this);
    char name[16];
    sprintf(name, "worker_%d", proxy_id);
    pthread_setname_np(worker_tid.native_handle(), name);
}

void cea_proxy::core::worker() {
    read_next_stream_from_stmq();
    extract_traffic_parameters();
    cur_stm->impl->prune();
    timer.start();
    cur_stm->impl->build_base_frame();
    cealog << "Timer taken by build_base_frame: " 
        << timer.elapsed_in_string() << endl;
    begin_mutation();
}

void cea_proxy::core::read_next_stream_from_stmq() {
    cur_stm = stmq[0];
}

void cea_proxy::core::extract_traffic_parameters() {
}

void cea_proxy::core::begin_mutation() {
    // write to fbuf
    // *(addr + i) = (char)i;
    // read from fbuf
    // if (*(addr + i) != (char)i)
}

void cea_proxy::core::create_frame_buffer() {
    fbuf = mmap(ADDR, LENGTH, PROTECTION, FLAGS, -1, 0);
    if (fbuf == MAP_FAILED) {
        CEA_MSG("***ERROR: Memory map failed");
        exit(1);
    }
}

void cea_proxy::core::release_frame_buffer() {
    if (munmap(fbuf, LENGTH)) {
        CEA_MSG("Error: Memory unmap failed");
        exit(1);
    }
}

//------------------------------------------------------------------------------
// Manager Implementation
//------------------------------------------------------------------------------

class cea_manager::core {
public:
    core();
    ~core();
    void add_proxy(cea_proxy *pxy);
    void add_proxy(cea_proxy *pxy, uint32_t cnt);
    void add_stream(cea_stream *stm, cea_proxy *pxy=NULL);
    void add_cmd(cea_stream *stm, cea_proxy *pxy=NULL);
    void exec_cmd(cea_stream *stm, cea_proxy *pxy=NULL);
    vector<cea_proxy*> proxies;
    string msg_prefix;
};

cea_manager::core::~core() = default;

cea_manager::cea_manager() : impl (new core) {
}

cea_manager::core::core() {
}

void cea_manager::add_proxy(cea_proxy *pxy) {
    impl->add_proxy(pxy);
}

void cea_manager::add_proxy(cea_proxy *pxy, uint32_t cnt) {
    impl->add_proxy(pxy, cnt);
}

void cea_manager::add_stream(cea_stream *stm, cea_proxy *pxy) {
    impl->add_stream(stm, pxy);
}

void cea_manager::add_cmd(cea_stream *stm, cea_proxy *pxy) {
    impl->add_cmd(stm, pxy);
}

void cea_manager::exec_cmd(cea_stream *stm, cea_proxy *pxy) {
    impl->exec_cmd(stm, pxy);
}

void cea_manager::core::add_proxy(cea_proxy *pxy) {
    proxies.push_back(pxy);
}

void cea_manager::core::add_proxy(cea_proxy *pxy, uint32_t cnt) {
    CEA_DBG(cnt << " Proxies added");
    for (uint32_t idx=0; idx<cnt; idx++)
        proxies.push_back(&pxy[idx]);
}

void cea_manager::core::add_stream(cea_stream *stm, cea_proxy *pxy) {
    if (pxy != NULL) {
        vector<cea_proxy*>::iterator it;
        for (it = proxies.begin(); it != proxies.end(); it++) {
            if ((*it)->impl->proxy_id == pxy->impl->proxy_id) {
                uint32_t idx = distance(proxies.begin(), it);
                proxies[idx]->add_stream(stm);
            }
        }
    } else {
        for (uint32_t idx=0; idx<proxies.size(); idx++) {
            proxies[idx]->add_stream(stm);
        }
    }
}

void cea_manager::core::add_cmd(cea_stream *stm, cea_proxy *pxy) {
    add_stream(stm, pxy);
}

void cea_manager::core::exec_cmd(cea_stream *stm, cea_proxy *pxy) {
}

} // namespace
