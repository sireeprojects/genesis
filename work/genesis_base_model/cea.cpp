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

#include "cea.h"

using namespace std;
using namespace chrono;

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

namespace cea {

uint32_t proxy_id = 0;
uint32_t stream_id = 0;

vector<cea_field> flds = {
//----------------------------------------------------------------------------------------------------------------------------------------------------
// Toc     Mrg Added    Stack Id                        Len       Offset Modifier Val                  Start Stop Step Rpt Name
//----------------------------------------------------------------------------------------------------------------------------------------------------
{  false,  0,  false,   0,    FRAME_Type               ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "FRAME_Type             ", TYPE_Integer },
{  false,  0,  false,   0,    FRAME_Len                ,0,        0,     Fixed,   64,                  0,    0,   0,   0,  "FRAME_Len              ", TYPE_Integer },
{  false,  0,  false,   0,    Network_Hdr              ,0,        0,     Fixed,   IPv4,                0,    0,   0,   0,  "Network_Hdr            ", TYPE_Integer },
{  false,  0,  false,   0,    Transport_Hdr            ,0,        0,     Fixed,   UDP,                 0,    0,   0,   0,  "Transport_Hdr          ", TYPE_Integer },
{  false,  0,  false,   0,    VLAN_Tag                 ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_Tag               ", TYPE_Integer },
{  false,  0,  false,   0,    MPLS_Hdr                 ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_Hdr               ", TYPE_Integer },
{  false,  0,  false,   0,    MAC_Preamble             ,64,       0,     Fixed,   0x55555555555555d5,  0,    0,   0,   0,  "MAC_Preamble           ", TYPE_Integer },
{  false,  0,  false,   0,    MAC_Dest_Addr            ,48,       0,     Fixed,   0x112233445566,      0,    0,   0,   0,  "MAC_Dest_Addr          ", TYPE_Special },
{  false,  0,  false,   0,    MAC_Src_Addr             ,48,       0,     Fixed,   0xaabbccddeeff,      0,    0,   0,   0,  "MAC_Src_Addr           ", TYPE_Special },
{  false,  0,  false,   0,    MAC_Len                  ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "MAC_Len                ", TYPE_Integer },
{  false,  0,  false,   0,    MAC_Ether_Type           ,16,       0,     Fixed,   0x0800,              0,    0,   0,   0,  "MAC_Ether_Type         ", TYPE_Integer },
{  false,  0,  false,   0,    MAC_Fcs                  ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "MAC_Fcs                ", TYPE_Integer },
{  false,  0,  false,   0,    LLC_Dsap                 ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "LLC_Dsap               ", TYPE_Integer },
{  false,  0,  false,   0,    LLC_Ssap                 ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "LLC_Ssap               ", TYPE_Integer },
{  false,  0,  false,   0,    LLC_Control              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "LLC_Control            ", TYPE_Integer },
{  false,  0,  false,   0,    SNAP_Oui                 ,24,       0,     Fixed,   0,                   0,    0,   0,   0,  "SNAP_Oui               ", TYPE_Integer },
{  false,  0,  false,   0,    SNAP_Pid                 ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "SNAP_Pid               ", TYPE_Integer },
{  false,  1,  false,   0,    IPv4_Version             ,4,        0,     Fixed,   4,                   0,    0,   0,   0,  "IPv4_Version           ", TYPE_Integer },
{  false,  1,  false,   0,    IPv4_IHL                 ,4,        0,     Fixed,   5,                   0,    0,   0,   0,  "IPv4_IHL               ", TYPE_Integer },
{  false,  0,  false,   0,    IPv4_Tos                 ,8,        0,     Fixed,   0xc0,                0,    0,   0,   0,  "IPv4_Tos               ", TYPE_Integer },
{  false,  0,  false,   0,    IPv4_Total_Len           ,16,       0,     Fixed,   0x33,                0,    0,   0,   0,  "IPv4_Total_Len         ", TYPE_Integer },
{  false,  0,  false,   0,    IPv4_Id                  ,16,       0,     Fixed,   0xaabb,              0,    0,   0,   0,  "IPv4_Id                ", TYPE_Integer },
{  false,  1,  false,   0,    IPv4_Flags               ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Flags             ", TYPE_Integer },
{  false,  1,  false,   0,    IPv4_Frag_Offset         ,13,       0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Frag_Offset       ", TYPE_Integer },
{  false,  0,  false,   0,    IPv4_TTL                 ,8,        0,     Fixed,   10,                  0,    0,   0,   0,  "IPv4_TTL               ", TYPE_Integer },
{  false,  0,  false,   0,    IPv4_Protocol            ,8,        0,     Fixed,   6,                   0,    0,   0,   0,  "IPv4_Protocol          ", TYPE_Integer },
{  false,  0,  false,   0,    IPv4_Hdr_Csum            ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Hdr_Csum          ", TYPE_Integer },
{  false,  0,  false,   0,    IPv4_Src_Addr            ,32,       0,     Fixed,   0x11223344,          0,    0,   0,   0,  "IPv4_Src_Addr          ", TYPE_Special },
{  false,  0,  false,   0,    IPv4_Dest_Addr           ,32,       0,     Fixed,   0xaabbccdd,          0,    0,   0,   0,  "IPv4_Dest_Addr         ", TYPE_Special },
{  false,  0,  false,   0,    IPv4_Opts                ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Opts              ", TYPE_Integer },
{  false,  0,  false,   0,    IPv4_Pad                 ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Pad               ", TYPE_Integer },
{  false,  2,  false,   0,    IPv6_Version             ,4,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Version           ", TYPE_Integer },
{  false,  1,  false,   0,    IPv6_Traffic_Class       ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Traffic_Class     ", TYPE_Integer },
{  false,  1,  false,   0,    IPv6_Flow_Label          ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Flow_Label        ", TYPE_Integer },
{  false,  0,  false,   0,    IPv6_Payload_Len         ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Payload_Len       ", TYPE_Integer },
{  false,  0,  false,   0,    IPv6_Next_Hdr            ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Next_Hdr          ", TYPE_Integer },
{  false,  0,  false,   0,    IPv6_Hop_Limit           ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Hop_Limit         ", TYPE_Integer },
{  false,  0,  false,   0,    IPv6_Src_Addr            ,128,      0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Src_Addr          ", TYPE_Integer },
{  false,  0,  false,   0,    IPv6_Dest_Addr           ,128,      0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Dest_Addr         ", TYPE_Integer },
{  false,  0,  false,   0,    TCP_Src_Port             ,16,       0,     Fixed,   1234,                0,    0,   0,   0,  "TCP_Src_Port           ", TYPE_Integer },
{  false,  0,  false,   0,    TCP_Dest_Port            ,16,       0,     Fixed,   5678,                0,    0,   0,   0,  "TCP_Dest_Port          ", TYPE_Integer },
{  false,  0,  false,   0,    TCP_Seq_Num              ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Seq_Num            ", TYPE_Integer },
{  false,  0,  false,   0,    TCP_Ack_Num              ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Ack_Num            ", TYPE_Integer },
{  false,  7,  false,   0,    TCP_Data_Offset          ,4,        0,     Fixed,   5,                   0,    0,   0,   0,  "TCP_Data_Offset        ", TYPE_Integer },
{  false,  1,  false,   0,    TCP_Reserved             ,6,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Reserved           ", TYPE_Integer },
{  false,  1,  false,   0,    TCP_Urg                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Urg                ", TYPE_Integer },
{  false,  1,  false,   0,    TCP_Ack                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Ack                ", TYPE_Integer },
{  false,  1,  false,   0,    TCP_Psh                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Psh                ", TYPE_Integer },
{  false,  1,  false,   0,    TCP_Rst                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Rst                ", TYPE_Integer },
{  false,  1,  false,   0,    TCP_Syn                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Syn                ", TYPE_Integer },
{  false,  1,  false,   0,    TCP_Fin                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Fin                ", TYPE_Integer },
{  false,  0,  false,   0,    TCP_Window               ,16,       0,     Fixed,   64,                  0,    0,   0,   0,  "TCP_Window             ", TYPE_Integer },
{  false,  0,  false,   0,    TCP_Csum                 ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Csum               ", TYPE_Integer },
{  false,  0,  false,   0,    TCP_Urg_Ptr              ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Urg_Ptr            ", TYPE_Integer },
{  false,  0,  false,   0,    TCP_Opts                 ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Opts               ", TYPE_Integer },
{  false,  0,  false,   0,    TCP_Pad                  ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Pad                ", TYPE_Integer },
{  false,  0,  false,   0,    UDP_Src_Port             ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Src_Port           ", TYPE_Integer },
{  false,  0,  false,   0,    UDP_Dest_Port            ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Dest_Port          ", TYPE_Integer },
{  false,  0,  false,   0,    UDP_Len                  ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Len                ", TYPE_Integer },
{  false,  0,  false,   0,    UDP_Csum                 ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Csum               ", TYPE_Integer },
{  false,  0,  false,   0,    ARP_Hw_Type              ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Hw_Type            ", TYPE_Integer },
{  false,  0,  false,   0,    ARP_Proto_Type           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Proto_Type         ", TYPE_Integer },
{  false,  0,  false,   0,    ARP_Hw_Len               ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Hw_Len             ", TYPE_Integer },
{  false,  0,  false,   0,    ARP_Proto_Len            ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Proto_Len          ", TYPE_Integer },
{  false,  0,  false,   0,    ARP_Opcode               ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Opcode             ", TYPE_Integer },
{  false,  0,  false,   0,    ARP_Sender_Hw_Addr       ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Sender_Hw_Addr     ", TYPE_Special },
{  false,  0,  false,   0,    ARP_Sender_Proto_addr    ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Sender_Proto_addr  ", TYPE_Special },
{  false,  0,  false,   0,    ARP_Target_Hw_Addr       ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Target_Hw_Addr     ", TYPE_Special },
{  false,  0,  false,   0,    ARP_Target_Proto_Addr    ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Target_Proto_Addr  ", TYPE_Special },
{  false,  0,  false,   0,    PAYLOAD_Type             ,46,       0,     Fixed,   0,                   0,    0,   0,   0,  "PAYLOAD_Type           ", TYPE_Integer },
{  false,  0,  false,   0,    PAYLOAD_Pattern          ,46,       0,     Fixed,   0,                   0,    0,   0,   0,  "PAYLOAD_Pattern        ", TYPE_Integer },
{  false,  0,  false,   0,    UDF1                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF1                   ", TYPE_Integer },
{  false,  0,  false,   0,    UDF2                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF2                   ", TYPE_Integer },
{  false,  0,  false,   0,    UDF3                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF3                   ", TYPE_Integer },
{  false,  0,  false,   0,    UDF4                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF4                   ", TYPE_Integer },
{  false,  0,  false,   0,    UDF5                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF5                   ", TYPE_Integer },
{  false,  0,  false,   0,    UDF6                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF6                   ", TYPE_Integer },
{  false,  0,  false,   0,    UDF7                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF7                   ", TYPE_Integer },
{  false,  0,  false,   0,    UDF8                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF8                   ", TYPE_Integer },
{  false,  2,  false,   1,    MPLS_01_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_01_Label          ", TYPE_Integer },
{  false,  1,  false,   1,    MPLS_01_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_01_Exp            ", TYPE_Integer },
{  false,  1,  false,   1,    MPLS_01_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_01_Stack          ", TYPE_Integer },
{  false,  0,  false,   1,    MPLS_01_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_01_Ttl            ", TYPE_Integer },
{  false,  2,  false,   2,    MPLS_02_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_02_Label          ", TYPE_Integer },
{  false,  1,  false,   2,    MPLS_02_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_02_Exp            ", TYPE_Integer },
{  false,  1,  false,   2,    MPLS_02_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_02_Stack          ", TYPE_Integer },
{  false,  0,  false,   2,    MPLS_02_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_02_Ttl            ", TYPE_Integer },
{  false,  2,  false,   3,    MPLS_03_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_03_Label          ", TYPE_Integer },
{  false,  1,  false,   3,    MPLS_03_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_03_Exp            ", TYPE_Integer },
{  false,  1,  false,   3,    MPLS_03_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_03_Stack          ", TYPE_Integer },
{  false,  0,  false,   3,    MPLS_03_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_03_Ttl            ", TYPE_Integer },
{  false,  2,  false,   4,    MPLS_04_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_04_Label          ", TYPE_Integer },
{  false,  1,  false,   4,    MPLS_04_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_04_Exp            ", TYPE_Integer },
{  false,  1,  false,   4,    MPLS_04_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_04_Stack          ", TYPE_Integer },
{  false,  0,  false,   4,    MPLS_04_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_04_Ttl            ", TYPE_Integer },
{  false,  2,  false,   5,    MPLS_05_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_05_Label          ", TYPE_Integer },
{  false,  1,  false,   5,    MPLS_05_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_05_Exp            ", TYPE_Integer },
{  false,  1,  false,   5,    MPLS_05_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_05_Stack          ", TYPE_Integer },
{  false,  0,  false,   5,    MPLS_05_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_05_Ttl            ", TYPE_Integer },
{  false,  2,  false,   6,    MPLS_06_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_06_Label          ", TYPE_Integer },
{  false,  1,  false,   6,    MPLS_06_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_06_Exp            ", TYPE_Integer },
{  false,  1,  false,   6,    MPLS_06_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_06_Stack          ", TYPE_Integer },
{  false,  0,  false,   6,    MPLS_06_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_06_Ttl            ", TYPE_Integer },
{  false,  2,  false,   7,    MPLS_07_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_07_Label          ", TYPE_Integer },
{  false,  1,  false,   7,    MPLS_07_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_07_Exp            ", TYPE_Integer },
{  false,  1,  false,   7,    MPLS_07_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_07_Stack          ", TYPE_Integer },
{  false,  0,  false,   7,    MPLS_07_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_07_Ttl            ", TYPE_Integer },
{  false,  2,  false,   8,    MPLS_08_Label            ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_08_Label          ", TYPE_Integer },
{  false,  1,  false,   8,    MPLS_08_Exp              ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_08_Exp            ", TYPE_Integer },
{  false,  1,  false,   8,    MPLS_08_Stack            ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_08_Stack          ", TYPE_Integer },
{  false,  0,  false,   8,    MPLS_08_Ttl              ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_08_Ttl            ", TYPE_Integer },
{  false,  0,  false,   1,    VLAN_01_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_01_Tpi            ", TYPE_Integer },
{  false,  2,  false,   1,    VLAN_01_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_01_Tci_Pcp        ", TYPE_Integer },
{  false,  1,  false,   1,    VLAN_01_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_01_Tci_Cfi        ", TYPE_Integer },
{  false,  1,  false,   1,    VLAN_01_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_01_Vid            ", TYPE_Integer },
{  false,  0,  false,   2,    VLAN_02_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_02_Tpi            ", TYPE_Integer },
{  false,  2,  false,   2,    VLAN_02_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_02_Tci_Pcp        ", TYPE_Integer },
{  false,  1,  false,   2,    VLAN_02_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_02_Tci_Cfi        ", TYPE_Integer },
{  false,  1,  false,   2,    VLAN_02_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_02_Vid            ", TYPE_Integer },
{  false,  0,  false,   3,    VLAN_03_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_03_Tpi            ", TYPE_Integer },
{  false,  2,  false,   3,    VLAN_03_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_03_Tci_Pcp        ", TYPE_Integer },
{  false,  1,  false,   3,    VLAN_03_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_03_Tci_Cfi        ", TYPE_Integer },
{  false,  1,  false,   3,    VLAN_03_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_03_Vid            ", TYPE_Integer },
{  false,  0,  false,   4,    VLAN_04_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_04_Tpi            ", TYPE_Integer },
{  false,  2,  false,   4,    VLAN_04_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_04_Tci_Pcp        ", TYPE_Integer },
{  false,  1,  false,   4,    VLAN_04_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_04_Tci_Cfi        ", TYPE_Integer },
{  false,  1,  false,   4,    VLAN_04_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_04_Vid            ", TYPE_Integer },
{  false,  0,  false,   5,    VLAN_05_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_05_Tpi            ", TYPE_Integer },
{  false,  2,  false,   5,    VLAN_05_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_05_Tci_Pcp        ", TYPE_Integer },
{  false,  1,  false,   5,    VLAN_05_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_05_Tci_Cfi        ", TYPE_Integer },
{  false,  1,  false,   5,    VLAN_05_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_05_Vid            ", TYPE_Integer },
{  false,  0,  false,   6,    VLAN_06_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_06_Tpi            ", TYPE_Integer },
{  false,  2,  false,   6,    VLAN_06_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_06_Tci_Pcp        ", TYPE_Integer },
{  false,  1,  false,   6,    VLAN_06_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_06_Tci_Cfi        ", TYPE_Integer },
{  false,  1,  false,   6,    VLAN_06_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_06_Vid            ", TYPE_Integer },
{  false,  0,  false,   7,    VLAN_07_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_07_Tpi            ", TYPE_Integer },
{  false,  2,  false,   7,    VLAN_07_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_07_Tci_Pcp        ", TYPE_Integer },
{  false,  1,  false,   7,    VLAN_07_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_07_Tci_Cfi        ", TYPE_Integer },
{  false,  1,  false,   7,    VLAN_07_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_07_Vid            ", TYPE_Integer },
{  false,  0,  false,   8,    VLAN_08_Tpi              ,16,       0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_08_Tpi            ", TYPE_Integer },
{  false,  2,  false,   8,    VLAN_08_Tci_Pcp          ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_08_Tci_Pcp        ", TYPE_Integer },
{  false,  1,  false,   8,    VLAN_08_Tci_Cfi          ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_08_Tci_Cfi        ", TYPE_Integer },
{  false,  1,  false,   8,    VLAN_08_Vid              ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_08_Vid            ", TYPE_Integer },
{  false,  0,  false,   0,    Num_VLAN_Tags            ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "Num_VLAN_Tags          ", TYPE_Integer },
{  false,  0,  false,   0,    Num_MPLS_Hdrs            ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "Num_MPLS_Hdrs          ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Type              ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Type            ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Pkts_Per_Burst    ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Pkts_Per_Burst  ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Burst_Per_Stream  ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Burst_Per_Stream", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Inter_Burst_Gap   ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Inter_Burst_Gap ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Inter_Stream_Gap  ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Inter_Stream_Gap", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Start_Delay       ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Start_Delay     ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Rate_Type         ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Rate_Type       ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Rate              ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Rate            ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Ipg               ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Ipg             ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Percentage        ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Percentage      ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Pkts_Per_Sec      ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Pkts_Per_Sec    ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Bit_Rate          ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Bit_Rate        ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Crc_Enable        ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Crc_Enable      ", TYPE_Integer },
{  false,  0,  false,   0,    STREAM_Timestamp_Enable  ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Timestamp_Enable", TYPE_Integer },
{  false,  0,  false,   0,    MAC_Control              ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "MAC_Control            ", TYPE_Integer },
{  false,  0,  false,   0,    MAC_Control_Opcode       ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "MAC_Control_Opcode     ", TYPE_Integer },
{  false,  0,  false,   0,    Pause_Quanta             ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta           ", TYPE_Integer },
{  false,  0,  false,   0,    Priority_En_Vector       ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Priority_En_Vector     ", TYPE_Integer },
{  false,  0,  false,   0,    Pause_Quanta_0           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_0         ", TYPE_Integer },
{  false,  0,  false,   0,    Pause_Quanta_1           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_1         ", TYPE_Integer },
{  false,  0,  false,   0,    Pause_Quanta_2           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_2         ", TYPE_Integer },
{  false,  0,  false,   0,    Pause_Quanta_3           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_3         ", TYPE_Integer },
{  false,  0,  false,   0,    Pause_Quanta_4           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_4         ", TYPE_Integer },
{  false,  0,  false,   0,    Pause_Quanta_5           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_5         ", TYPE_Integer },
{  false,  0,  false,   0,    Pause_Quanta_6           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_6         ", TYPE_Integer },
{  false,  0,  false,   0,    Pause_Quanta_7           ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "Pause_Quanta_7         ", TYPE_Integer },
{  false,  0,  false,   0,    Zeros_8Bit               ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "Zeros_8Bit             ", TYPE_Integer },
{  false,  0,  false,   0,    TCP_Total_Len            ,16,       0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Total_Len          ", TYPE_Integer },
{  false,  0,  false,   0,    META_Len                 ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "META_Len               ", TYPE_Integer },
{  false,  0,  false,   0,    META_Ipg                 ,32,       0,     Fixed,   0,                   0,    0,   0,   0,  "META_Ipg               ", TYPE_Integer },
{  false,  0,  false,   0,    META_Preamble            ,64,       0,     Fixed,   0,                   0,    0,   0,   0,  "META_Preamble          ", TYPE_Integer },
{  false,  0,  false,   0,    META_Pad1                ,64,       0,     Fixed,   0,                   0,    0,   0,   0,  "META_Pad1              ", TYPE_Integer },
{  false,  0,  false,   0,    META_Pad2                ,64,       0,     Fixed,   0,                   0,    0,   0,   0,  "META_Pad1              ", TYPE_Integer },
{  false,  0,  false,   0,    META_Pad3                ,64,       0,     Fixed,   0,                   0,    0,   0,   0,  "META_Pad1              ", TYPE_Integer },
{  false,  0,  false,   0,    META_Pad4                ,64,       0,     Fixed,   0,                   0,    0,   0,   0,  "META_Pad1              ", TYPE_Integer },
{  false,  0,  false,   0,    META_Pad5                ,64,       0,     Fixed,   0,                   0,    0,   0,   0,  "META_Pad1              ", TYPE_Integer },
{  false,  0,  false,   0,    META_Pad6                ,64,       0,     Fixed,   0,                   0,    0,   0,   0,  "META_Pad1              ", TYPE_Integer }
};

// header to fields map (field groups)
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

ofstream logfile;
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

//------------------------------------------------------------------------------
// Stream implementation
//------------------------------------------------------------------------------

cea_stream::~cea_stream() = default;

void cea_stream::print_stream_properties() {
}

// constructor
cea_stream::cea_stream(string name) {
    stream_id = cea::stream_id;
    cea::stream_id++;
    stream_name = name + ":" + to_string(stream_id);
    reset();
}

void cea_proxy::reset() {
    msg_prefix = '(' + proxy_name + ") ";
}


void cea_stream::reset() {
    msg_prefix = '(' + stream_name + ") ";
}

// copy constructor
cea_stream::cea_stream(const cea_stream &rhs) {
    // impl->do_copy(&rhs); // TODO
}

// assign operator overload
cea_stream& cea_stream::operator = (cea_stream &rhs) {
   if (this != &rhs) {
      // impl->do_copy(&rhs);
   }
   return *this;
}

// print operator overload
ostream& operator << (ostream &os, const cea_stream &f) {
    // os << f.impl->describe();
    return os;
}

// utility
bool in_range(uint32_t low, uint32_t high, uint32_t x) {        
    return (low <= x && x <= high);         
}

void cea_stream::set(cea_field_id id, uint64_t value) {
    // impl->set(id, value);
}

void cea_stream::set(cea_field_id id, cea_field_generation_spec spec) {
    // impl->set(id, spec);
}

void cea_stream::do_copy(const cea_stream *rhs) {
}

cea_proxy::~cea_proxy() = default;

void cea_proxy::add_stream(cea_stream *stm) {
    // impl->stmq.push_back(stm);
}

void cea_proxy::add_cmd(cea_stream *stm) {
    // impl->stmq.push_back(stm);
}

void cea_proxy::exec_cmd(cea_stream *stm) {
    // impl->exec_cmd(stm);
}

cea_proxy::cea_proxy(string name) {
    proxy_id = cea::proxy_id;
    cea::proxy_id++;
    proxy_name = name + ":" + to_string(proxy_id);
    reset();
    CEA_MSG("Proxy created with name=" << name << " and id=" << proxy_id);
}

void cea_proxy::start() {
}

} // namespace
