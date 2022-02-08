/*------------------------------------------------------------------------------
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
- 
------------------------------------------------------------------------------*/

    // s.add(VLAN_Tags);
    // s.add(MPLS_Labels);
#include "cea.h"

#define CEA_PXY_DBG_CALL_SIGNATURE CEA_DBG( \
    "(%s) Fn:%s: Invoked", proxy_name.c_str(), __FUNCTION__);

#define CEA_STREAM_DBG_CALL_SIGNATURE CEA_DBG( \
    "(%s) Fn:%s: Invoked", stream_name.c_str(), __FUNCTION__);

#define CEA_MSG(...) \
    cealog << string_format(__VA_ARGS__) << endl; 

#ifdef CEA_DEBUG
    #define CEA_DBG(...) { CEA_MSG(__VA_ARGS__) }
#else
    #define CEA_DBG(...) {}
#endif

// 1GB frame buffer with read and write capabilities
#define LENGTH (1UL*1024*1024*1024)
#define PROTECTION (PROT_READ | PROT_WRITE)

#ifndef MAP_HUGETLB
#define MAP_HUGETLB 0x40000 /* arch specific */
#endif

// Only ia64 requires this 
#ifdef __ia64__
#define ADDR (void *)(0x8000000000000000UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_FIXED)
#else
#define ADDR (void *)(0x0UL)
#define FLAGS (MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB)
#endif

// maximum pkt size from mac dest addr to mac crc (16KB)
#define CEA_MAX_PKT_SIZE 16384

// stringize a printf like formatted output 
template<typename ... Args>
string string_format(const string& format, Args ... args) {
    size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1;
    if(size <= 0) {
        throw runtime_error("Error during formatting.");
    }
    unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format.c_str(), args ...);
    return string(buf.get(), buf.get() + size - 1);
}

namespace cea {

cea_field flds[] = {
// TODO multiple VLAN tags, multiple mpls labels, ip/tcp/udp checksum, multiple UDF fields
//------------------------------------------------------------------------------------------------------------------------------------------
// Toc     Mrg Mask Id                        Len       Offset Modifier Val                  Start Stop Step Rpt Name
//------------------------------------------------------------------------------------------------------------------------------------------
{  false,  0,  0,   PKT_Type                 ,0,        0,     Fixed,   ETH_V2,              0,    0,   0,   0,  "PKT_Type               "},
{  false,  0,  0,   Network_Hdr              ,0,        0,     Fixed,   IPv4,                0,    0,   0,   0,  "Network_Hdr            "},
{  false,  0,  0,   Transport_Hdr            ,0,        0,     Fixed,   UDP,                 0,    0,   0,   0,  "Transport_Hdr          "},
{  false,  0,  0,   VLAN_Tags                ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_Tags              "},
{  false,  0,  0,   MPLS_Labels              ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_Labels            "},
{  false,  0,  0,   MAC_Preamble             ,8*8,      0,     Fixed,   0x5555555555d5,      0,    0,   0,   0,  "MAC_Preamble           "},
{  false,  0,  0,   MAC_Dest_Addr            ,8*6,      0,     Fixed,   0x112233445566,      0,    0,   0,   0,  "MAC_Dest_Addr          "},
{  false,  0,  0,   MAC_Src_Addr             ,8*6,      0,     Fixed,   0xaabbccddeeff,      0,    0,   0,   0,  "MAC_Src_Addr           "},
{  false,  0,  0,   MAC_Len                  ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "MAC_Len                "},
{  false,  0,  0,   MAC_Ether_Type           ,8*2,      0,     Fixed,   0x0800,              0,    0,   0,   0,  "MAC_Ether_Type         "},
{  false,  0,  0,   MAC_Fcs                  ,8*4,      0,     Fixed,   0,                   0,    0,   0,   0,  "MAC_Fcs                "},
{  false,  0,  0,   VLAN_Tpi                 ,8*2,      0,     Fixed,   0x8100,              0,    0,   0,   0,  "VLAN_Tpi               "},
{  false,  2,  0,   VLAN_Tci_Pcp             ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_Tci_Pcp           "},
{  false,  1,  0,   VLAN_Tci_Cfi             ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_Tci_Cfi           "},
{  false,  1,  0,   VLAN_Vid                 ,12,       0,     Fixed,   0,                   0,    0,   0,   0,  "VLAN_Vid               "},
{  false,  0,  0,   LLC_Dsap                 ,8*1,      0,     Fixed,   0,                   0,    0,   0,   0,  "LLC_Dsap               "},
{  false,  0,  0,   LLC_Ssap                 ,8*1,      0,     Fixed,   0,                   0,    0,   0,   0,  "LLC_Ssap               "},
{  false,  0,  0,   LLC_Control              ,8*1,      0,     Fixed,   0,                   0,    0,   0,   0,  "LLC_Control            "},
{  false,  0,  0,   SNAP_Oui                 ,8*3,      0,     Fixed,   0,                   0,    0,   0,   0,  "SNAP_Oui               "},
{  false,  0,  0,   SNAP_Pid                 ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "SNAP_Pid               "},
{  false,  2,  0,   MPLS_Label               ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_Label             "},
{  false,  1,  0,   MPLS_Cos                 ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_Cos               "},
{  false,  1,  0,   MPLS_Stack               ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_Stack             "},
{  false,  0,  0,   MPLS_Ttl                 ,8*1,      0,     Fixed,   0,                   0,    0,   0,   0,  "MPLS_Ttl               "},
{  false,  1,  0,   IPv4_Version             ,4,        0,     Fixed,   4,                   0,    0,   0,   0,  "IPv4_Version           "},
{  false,  1,  0,   IPv4_IHL                 ,4,        0,     Fixed,   5,                   0,    0,   0,   0,  "IPv4_IHL               "},
{  false,  0,  0,   IPv4_Tos                 ,8*1,      0,     Fixed,   0xc0,                0,    0,   0,   0,  "IPv4_Tos               "},
{  false,  0,  0,   IPv4_Total_Len           ,8*2,      0,     Fixed,   0x33,                0,    0,   0,   0,  "IPv4_Total_Len         "},
{  false,  0,  0,   IPv4_Id                  ,8*2,      0,     Fixed,   0xaabb,              0,    0,   0,   0,  "IPv4_Id                "},
{  false,  1,  0,   IPv4_Flags               ,3,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Flags             "},
{  false,  1,  0,   IPv4_Frag_Offset         ,13,       0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Frag_Offset       "},
{  false,  0,  0,   IPv4_TTL                 ,8*1,      0,     Fixed,   10,                  0,    0,   0,   0,  "IPv4_TTL               "},
{  false,  0,  0,   IPv4_Protocol            ,8*1,      0,     Fixed,   17,                  0,    0,   0,   0,  "IPv4_Protocol          "},
{  false,  0,  0,   IPv4_Hdr_Csum            ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Hdr_Csum          "},
{  false,  0,  0,   IPv4_Src_Addr            ,8*4,      0,     Fixed,   0x11223344,          0,    0,   0,   0,  "IPv4_Src_Addr          "},
{  false,  0,  0,   IPv4_Dest_Addr           ,8*4,      0,     Fixed,   0xaabbccdd,          0,    0,   0,   0,  "IPv4_Dest_Addr         "},
{  false,  0,  0,   IPv4_Opts                ,8*0,      0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Opts              "},
{  false,  0,  0,   IPv4_Pad                 ,8*0,      0,     Fixed,   0,                   0,    0,   0,   0,  "IPv4_Pad               "},
{  false,  2,  0,   IPv6_Version             ,4,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Version           "},
{  false,  1,  0,   IPv6_Traffic_Class       ,8,        0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Traffic_Class     "},
{  false,  1,  0,   IPv6_Flow_Label          ,20,       0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Flow_Label        "},
{  false,  0,  0,   IPv6_Payload_Len         ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Payload_Len       "},
{  false,  0,  0,   IPv6_Next_Hdr            ,8*1,      0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Next_Hdr          "},
{  false,  0,  0,   IPv6_Hop_Limit           ,8*1,      0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Hop_Limit         "},
{  false,  0,  0,   IPv6_Src_Addr            ,8*16,     0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Src_Addr          "},
{  false,  0,  0,   IPv6_Dest_Addr           ,8*16,     0,     Fixed,   0,                   0,    0,   0,   0,  "IPv6_Dest_Addr         "},
{  false,  0,  0,   TCP_Src_Port             ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Src_Port           "},
{  false,  0,  0,   TCP_Dest_Port            ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Dest_Port          "},
{  false,  0,  0,   TCP_Seq_Num              ,8*4,      0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Seq_Num            "},
{  false,  0,  0,   TCP_Ack_Num              ,8*4,      0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Ack_Num            "},
{  false,  7,  0,   TCP_Data_Offset          ,4,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Data_Offset        "},
{  false,  1,  0,   TCP_Reserved             ,6,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Reserved           "},
{  false,  1,  0,   TCP_Urg                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Urg                "},
{  false,  1,  0,   TCP_Ack                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Ack                "},
{  false,  1,  0,   TCP_Psh                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Psh                "},
{  false,  1,  0,   TCP_Rst                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Rst                "},
{  false,  1,  0,   TCP_Syn                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Syn                "},
{  false,  1,  0,   TCP_Fin                  ,1,        0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Fin                "},
{  false,  0,  0,   TCP_Window               ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Window             "},
{  false,  0,  0,   TCP_Csum                 ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Csum               "},
{  false,  0,  0,   TCP_Urg_Ptr              ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Urg_Ptr            "},
{  false,  0,  0,   TCP_Opts                 ,8*0,      0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Opts               "},
{  false,  0,  0,   TCP_Pad                  ,8*0,      0,     Fixed,   0,                   0,    0,   0,   0,  "TCP_Pad                "},
{  false,  0,  0,   UDP_Src_Port             ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Src_Port           "},
{  false,  0,  0,   UDP_Dest_Port            ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Dest_Port          "},
{  false,  0,  0,   UDP_Len                  ,8*2,      0,     Fixed,   0x1f,                0,    0,   0,   0,  "UDP_Len                "},
{  false,  0,  0,   UDP_Csum                 ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "UDP_Csum               "},
{  false,  0,  0,   ARP_Hw_Type              ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Hw_Type            "},
{  false,  0,  0,   ARP_Proto_Type           ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Proto_Type         "},
{  false,  0,  0,   ARP_Hw_Len               ,8*1,      0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Hw_Len             "},
{  false,  0,  0,   ARP_Proto_Len            ,8*1,      0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Proto_Len          "},
{  false,  0,  0,   ARP_Opcode               ,8*2,      0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Opcode             "},
{  false,  0,  0,   ARP_Sender_Hw_Addr       ,8*4,      0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Sender_Hw_Addr     "},
{  false,  0,  0,   ARP_Sender_Proto_addr    ,8*4,      0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Sender_Proto_addr  "},
{  false,  0,  0,   ARP_Target_Hw_Addr       ,8*4,      0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Target_Hw_Addr     "},
{  false,  0,  0,   ARP_Target_Proto_Addr    ,8*4,      0,     Fixed,   0,                   0,    0,   0,   0,  "ARP_Target_Proto_Addr  "},
{  false,  0,  0,   PAYLOAD_Type             ,46,       0,     Fixed,   0,                   0,    0,   0,   0,  "PAYLOAD_Type           "},
{  false,  0,  0,   PAYLOAD_Len              ,46,       0,     Fixed,   0,                   0,    0,   0,   0,  "PAYLOAD_Len            "},
{  false,  0,  0,   UDF1                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF1                   "},
{  false,  0,  0,   UDF2                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF2                   "},
{  false,  0,  0,   UDF3                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF3                   "},
{  false,  0,  0,   UDF4                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF4                   "},
{  false,  0,  0,   UDF5                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF5                   "},
{  false,  0,  0,   UDF6                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF6                   "},
{  false,  0,  0,   UDF7                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF7                   "},
{  false,  0,  0,   UDF8                     ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "UDF8                   "},
{  false,  0,  0,   STREAM_Type              ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Type            "},
{  false,  0,  0,   STREAM_Pkts_Per_Burst    ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Pkts_Per_Burst  "},
{  false,  0,  0,   STREAM_Burst_Per_Stream  ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Burst_Per_Stream"},
{  false,  0,  0,   STREAM_Inter_Burst_Gap   ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Inter_Burst_Gap "},
{  false,  0,  0,   STREAM_Inter_Stream_Gap  ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Inter_Stream_Gap"},
{  false,  0,  0,   STREAM_Start_Delay       ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Start_Delay     "},
{  false,  0,  0,   STREAM_Rate_Type         ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Rate_Type       "},
{  false,  0,  0,   STREAM_Rate              ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Rate            "},
{  false,  0,  0,   STREAM_Ipg               ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Ipg             "},
{  false,  0,  0,   STREAM_Percentage        ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Percentage      "},
{  false,  0,  0,   STREAM_Pkts_Per_Sec      ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Pkts_Per_Sec    "},
{  false,  0,  0,   STREAM_Bit_Rate          ,0,        0,     Fixed,   0,                   0,    0,   0,   0,  "STREAM_Bit_Rate        "},
//
};

// header to fields map
// preamble, ether type and len will be added during the formation of fseq
map <cea_hdr_type, vector <cea_field_id>> htof = {
    {MAC,   {
            MAC_Dest_Addr,
            MAC_Src_Addr
            }},
    {VLAN,  {
            VLAN_Tpi,
            VLAN_Tci_Pcp,
            VLAN_Tci_Cfi,
            VLAN_Vid
            }},
    {MPLS,  {
            MPLS_Label,
            MPLS_Cos,
            MPLS_Stack,
            MPLS_Ttl
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
    {UDP, { UDP_Src_Port,
            UDP_Dest_Port,
            UDP_Len,
            UDP_Csum,
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
            cout << "Error creating logfile. Aborting..." << endl;
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

// pcap per packet header
struct CEA_PACKED pcap_pkt_hdr {
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

void write_pcap(unsigned char *pkt, uint32_t len) {
    pcap_file_hdr fh;
    pcap_pkt_hdr ph;

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

    char buf[CEA_MAX_PKT_SIZE];
    uint32_t offset = 0;

    ofstream pcapfile;

    if (!file_exists("run.pcap")) {
        pcapfile.open("run.pcap", ofstream::app);
        pcapfile.write((char*)&fh, sizeof(pcap_file_hdr));
    } else {
        pcapfile.open("run.pcap", ofstream::app);
    }

    memcpy(buf+offset, (char*)&ph, sizeof(pcap_pkt_hdr));
    offset += sizeof(pcap_pkt_hdr);
    memcpy(buf+offset, pkt, len);
    offset += len;

    pcapfile.write(buf, offset);
    pcapfile.close();
}

// memcpy in network byte order
void *cea_memcpy_ntw_byte_order (void *dest, const void *src, size_t len) {
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

// TODO tcp checksum 
void compute_tcp_csum(char *hdr) {
}

// TODO udp checksum 
void compute_udp_csum(char *hdr) {
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

//  break a long string to a multiline string by breaking 
//  the input string to an approximate length of line_width
//  characters adding four leading spaces before each break
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

// stringize cea_pkt_type
string to_str(cea_pkt_type t) {
    string name;
    switch(t) {
        case ETH_V2   : { name = "ETH_V2  "; break; }
        case ETH_LLC  : { name = "ETH_LLC "; break; }
        case ETH_SNAP : { name = "ETH_SNAP"; break; }
        default            : { name = "undefined    "; break; }
    }
    return cea_trim(name);
}

// stringize cea_hdr_type
string to_str(cea_hdr_type t) {
    string name;
    switch(t) {
        case MAC    : { name = "MAC "; break; }
        case VLAN   : { name = "VLAN"; break; }
        case MPLS   : { name = "MPLS"; break; }
        case LLC    : { name = "LLC "; break; }
        case SNAP   : { name = "SNAP"; break; }
        case IPv4   : { name = "IPv4"; break; }
        case IPv6   : { name = "IPv6"; break; }
        case ARP    : { name = "ARP "; break; }
        case TCP    : { name = "TCP "; break; }
        case UDP    : { name = "UDP "; break; }
        default     : { name = "undefined"; break; }
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
        default                : { name = "UNDEFINED        "; break; }
    }
    return cea_trim(name);
}

// stringize cea_field_id
string to_str(cea_field_id t) {
    string name;
    switch(t) {
        case PKT_Type                : { name = "PKT_Type               "; break; }
        case Network_Hdr             : { name = "Network_Hdr            "; break; }
        case Transport_Hdr           : { name = "Transport_Hdr          "; break; }
        case VLAN_Tags               : { name = "VLAN_Tags              "; break; }
        case MPLS_Labels             : { name = "MPLS_Labels            "; break; }
        case MAC_Preamble            : { name = "MAC_Preamble           "; break; }
        case MAC_Dest_Addr           : { name = "MAC_Dest_Addr          "; break; }
        case MAC_Src_Addr            : { name = "MAC_Src_Addr           "; break; }
        case MAC_Len                 : { name = "MAC_Len                "; break; }
        case MAC_Ether_Type          : { name = "MAC_Ether_Type         "; break; }
        case MAC_Fcs                 : { name = "MAC_Fcs                "; break; }
        case VLAN_Tpi                : { name = "VLAN_Tpi               "; break; }
        case VLAN_Tci_Pcp            : { name = "VLAN_Tci_Pcp           "; break; }
        case VLAN_Tci_Cfi            : { name = "VLAN_Tci_Cfi           "; break; }
        case VLAN_Vid                : { name = "VLAN_Vid               "; break; }
        case LLC_Dsap                : { name = "LLC_Dsap               "; break; }
        case LLC_Ssap                : { name = "LLC_Ssap               "; break; }
        case LLC_Control             : { name = "LLC_Control            "; break; }
        case SNAP_Oui                : { name = "SNAP_Oui               "; break; }
        case SNAP_Pid                : { name = "SNAP_Pid               "; break; }
        case MPLS_Label              : { name = "MPLS_Label             "; break; }
        case MPLS_Cos                : { name = "MPLS_Cos               "; break; }
        case MPLS_Stack              : { name = "MPLS_Stack             "; break; }
        case MPLS_Ttl                : { name = "MPLS_Ttl               "; break; }
        case IPv4_Version            : { name = "IPv4_Version           "; break; }
        case IPv4_IHL                : { name = "IPv4_IHL               "; break; }
        case IPv4_Tos                : { name = "IPv4_Tos               "; break; }
        case IPv4_Total_Len          : { name = "IPv4_Total_Len         "; break; }
        case IPv4_Id                 : { name = "IPv4_Id                "; break; }
        case IPv4_Flags              : { name = "IPv4_Flags             "; break; }
        case IPv4_Frag_Offset        : { name = "IPv4_Frag_Offset       "; break; }
        case IPv4_TTL                : { name = "IPv4_TTL               "; break; }
        case IPv4_Protocol           : { name = "IPv4_Protocol          "; break; }
        case IPv4_Hdr_Csum           : { name = "IPv4_Hdr_Csum          "; break; }
        case IPv4_Src_Addr           : { name = "IPv4_Src_Addr          "; break; }
        case IPv4_Dest_Addr          : { name = "IPv4_Dest_Addr         "; break; }
        case IPv4_Opts               : { name = "IPv4_Opts              "; break; }
        case IPv4_Pad                : { name = "IPv4_Pad               "; break; }
        case IPv6_Version            : { name = "IPv6_Version           "; break; }
        case IPv6_Traffic_Class      : { name = "IPv6_Traffic_Class     "; break; }
        case IPv6_Flow_Label         : { name = "IPv6_Flow_Label        "; break; }
        case IPv6_Payload_Len        : { name = "IPv6_Payload_Len       "; break; }
        case IPv6_Next_Hdr           : { name = "IPv6_Next_Hdr          "; break; }
        case IPv6_Hop_Limit          : { name = "IPv6_Hop_Limit         "; break; }
        case IPv6_Src_Addr           : { name = "IPv6_Src_Addr          "; break; }
        case IPv6_Dest_Addr          : { name = "IPv6_Dest_Addr         "; break; }
        case TCP_Src_Port            : { name = "TCP_Src_Port           "; break; }
        case TCP_Dest_Port           : { name = "TCP_Dest_Port          "; break; }
        case TCP_Seq_Num             : { name = "TCP_Seq_Num            "; break; }
        case TCP_Ack_Num             : { name = "TCP_Ack_Num            "; break; }
        case TCP_Data_Offset         : { name = "TCP_Data_Offset        "; break; }
        case TCP_Reserved            : { name = "TCP_Reserved           "; break; }
        case TCP_Urg                 : { name = "TCP_Urg                "; break; }
        case TCP_Ack                 : { name = "TCP_Ack                "; break; }
        case TCP_Psh                 : { name = "TCP_Psh                "; break; }
        case TCP_Rst                 : { name = "TCP_Rst                "; break; }
        case TCP_Syn                 : { name = "TCP_Syn                "; break; }
        case TCP_Fin                 : { name = "TCP_Fin                "; break; }
        case TCP_Window              : { name = "TCP_Window             "; break; }
        case TCP_Csum                : { name = "TCP_Csum               "; break; }
        case TCP_Urg_Ptr             : { name = "TCP_Urg_Ptr            "; break; }
        case TCP_Opts                : { name = "TCP_Opts               "; break; }
        case TCP_Pad                 : { name = "TCP_Pad                "; break; }
        case UDP_Src_Port            : { name = "UDP_Src_Port           "; break; }
        case UDP_Dest_Port           : { name = "UDP_Dest_Port          "; break; }
        case UDP_Len                 : { name = "UDP_Len                "; break; }
        case UDP_Csum                : { name = "UDP_Csum               "; break; }
        case ARP_Hw_Type             : { name = "ARP_Hw_Type            "; break; }
        case ARP_Proto_Type          : { name = "ARP_Proto_Type         "; break; }
        case ARP_Hw_Len              : { name = "ARP_Hw_Len             "; break; }
        case ARP_Proto_Len           : { name = "ARP_Proto_Len          "; break; }
        case ARP_Opcode              : { name = "ARP_Opcode             "; break; }
        case ARP_Sender_Hw_Addr      : { name = "ARP_Sender_Hw_Addr     "; break; }
        case ARP_Sender_Proto_addr   : { name = "ARP_Sender_Proto_addr  "; break; }
        case ARP_Target_Hw_Addr      : { name = "ARP_Target_Hw_Addr     "; break; }
        case ARP_Target_Proto_Addr   : { name = "ARP_Target_Proto_Addr  "; break; }
        case PAYLOAD_Type            : { name = "PAYLOAD_Type           "; break; }
        case PAYLOAD_Len             : { name = "PAYLOAD_Len            "; break; }
        case UDF1                    : { name = "UDF1                   "; break; }
        case UDF2                    : { name = "UDF2                   "; break; }
        case UDF3                    : { name = "UDF3                   "; break; }
        case UDF4                    : { name = "UDF4                   "; break; }
        case UDF5                    : { name = "UDF5                   "; break; }
        case UDF6                    : { name = "UDF6                   "; break; }
        case UDF7                    : { name = "UDF7                   "; break; }
        case UDF8                    : { name = "UDF8                   "; break; }
        case STREAM_Type             : { name = "STREAM_Type            "; break; }
        case STREAM_Pkts_Per_Burst   : { name = "STREAM_Pkts_Per_Burst  "; break; }
        case STREAM_Burst_Per_Stream : { name = "STREAM_Burst_Per_Stream"; break; }
        case STREAM_Inter_Burst_Gap  : { name = "STREAM_Inter_Burst_Gap "; break; }
        case STREAM_Inter_Stream_Gap : { name = "STREAM_Inter_Stream_Gap"; break; }
        case STREAM_Start_Delay      : { name = "STREAM_Start_Delay     "; break; }
        case STREAM_Rate_Type        : { name = "STREAM_Rate_Type       "; break; }
        case STREAM_Rate             : { name = "STREAM_Rate            "; break; }
        case STREAM_Ipg              : { name = "STREAM_Ipg             "; break; }
        case STREAM_Percentage       : { name = "STREAM_Percentage      "; break; }
        case STREAM_Pkts_Per_Sec     : { name = "STREAM_Pkts_Per_Sec    "; break; }
        case STREAM_Bit_Rate         : { name = "STREAM_Bit_Rate        "; break; }
        default                      : { name = "undefined              "; break; }
    }
    return cea_trim(name);
}

//------------------------------------------------------------------------------
// Manager
//------------------------------------------------------------------------------

cea_manager::cea_manager() {
    CEA_DBG("%s called", __FUNCTION__);
}

void cea_manager::add_proxy(cea_proxy *pxy) {
    CEA_DBG("%s called", __FUNCTION__);
    proxies.push_back(pxy);
}

void cea_manager::add_proxy(cea_proxy *pxy, uint32_t cnt) {
    CEA_DBG("%s called with %d proxies", __FUNCTION__, cnt);
    for(uint32_t idx=0; idx<cnt; idx++)
        proxies.push_back(&pxy[idx]);
}

void cea_manager::add_stream(cea_stream *stm, cea_proxy *pxy) {
    CEA_DBG("%s called", __FUNCTION__);
    if (pxy != NULL) {
        vector<cea_proxy*>::iterator it;
        for (it = proxies.begin(); it != proxies.end(); it++) {
            if ((*it)->proxy_id == pxy->proxy_id) {
                uint32_t idx = distance(proxies.begin(), it);
                proxies[idx]->add_stream(stm);
            }
        }
    } else {
        for(uint32_t idx=0; idx<proxies.size(); idx++) {
            proxies[idx]->add_stream(stm);
        }
    }
}

void cea_manager::add_cmd(cea_stream *stm, cea_proxy *pxy) {
    CEA_DBG("%s called", __FUNCTION__);
    add_stream(stm, pxy);
}

void cea_manager::exec_cmd(cea_stream *stm, cea_proxy *pxy) {
    CEA_DBG("%s called", __FUNCTION__);
}

//------------------------------------------------------------------------------
// Proxy
//------------------------------------------------------------------------------

// constructor
cea_proxy::cea_proxy(string name) {
    proxy_id = cea::proxy_id;
    cea::proxy_id++;
    proxy_name = name + ":" + to_string(proxy_id);
    CEA_DBG("(%s) Fn:%s: ProxyID: %d", proxy_name.c_str(), __FUNCTION__, proxy_id);
}

void cea_proxy::add_stream(cea_stream *stm) {
    stmq.push_back(stm);
}

void cea_proxy::add_cmd(cea_stream *stm) {
    stmq.push_back(stm);
}

void cea_proxy::exec_cmd(cea_stream *stm) {
}

// start stream processing and generate frames
void cea_proxy::start() {
    start_worker();
    join_threads();
}

void cea_proxy::join_threads() {
    worker_tid.join();
}

void cea_proxy::start_worker() {
    worker_tid = thread (&cea_proxy::worker, this);
    char name[16];
    sprintf(name, "worker_%d", proxy_id);
    pthread_setname_np(worker_tid.native_handle(), name);
}

void cea_proxy::worker() {
    read_next_stream_from_stmq();
    extract_traffic_parameters();
    cur_stm->prune_stream();
    cur_stm->build_baseline_pkt();
    generate_traffic();
}

void cea_proxy::read_next_stream_from_stmq() {
    CEA_PXY_DBG_CALL_SIGNATURE;
    cur_stm = stmq[0];
}

void cea_proxy::extract_traffic_parameters() {
    CEA_PXY_DBG_CALL_SIGNATURE;
}

void cea_proxy::generate_traffic() {
    CEA_PXY_DBG_CALL_SIGNATURE;
    // write to pbuf
    // *(addr + i) = (char)i;
    // read from pbuf
    // if (*(addr + i) != (char)i)
}

void cea_proxy::create_pkt_buffer() {
    pbuf = mmap(ADDR, LENGTH, PROTECTION, FLAGS, -1, 0);
    if (pbuf == MAP_FAILED) {
        CEA_MSG("Error: Memory map failed in __FUNCTION__");
        exit(1);
    }
}

void cea_proxy::release_pkt_buffer() {
    if (munmap(pbuf, LENGTH)) {
        CEA_MSG("Error: Memory unmap failed in __FUNCTION__");
        exit(1);
    }
}

//------------------------------------------------------------------------------
// Stream
//------------------------------------------------------------------------------

void cea_stream::prune_stream() {
    organize_fields();
    trim_static_fields();
}

void cea_stream::build_baseline_pkt() {
    CEA_STREAM_DBG_CALL_SIGNATURE;

    // TODO: compute this correctly; 65 is for test purpose only
    baseline_pkt_len = 65;
    
    uint32_t offset = 0;
    uint64_t merged = 0;
    uint64_t tmp =  0;
    uint64_t len = 0;
    uint64_t mlen = 0;

    for (uint32_t i=0; i<fseq.size(); i++) {
        if (fields[fseq[i]].merge != 0) {
            merged = fields[fseq[i]].value; // first field
            mlen += fields[fseq[i]].len;
            for(uint32_t x=(i+1); x<=((i+fields[fseq[i]].merge)); x++) {
                len = fields[fseq[x]].len;
                merged = (merged << len) | fields[fseq[x]].value;
                mlen += len;
            }
            cea_memcpy_ntw_byte_order(base_pkt+offset, (char*)&merged, mlen/8);
            offset += mlen/8;
            i += fields[fseq[i]].merge; // skip mergable entries
        } else {
            uint64_t tmp = fields[fseq[i]].value;
            uint64_t len = fields[fseq[i]].len;
            cea_memcpy_ntw_byte_order(base_pkt+offset, (char*)&tmp, len/8);
            offset += len/8;
            mlen = 0; // TODO fix this
            merged = 0; // TODO fix this
        }
    }

    // baseline_pkt_len = offset; // TODO: fix this

    // insert IPv4 checksum
    uint16_t ipcsum = compute_ipv4_csum(base_pkt+14, 20);
    memcpy(base_pkt+24, (char*)&ipcsum, 2);

    #ifdef CEA_DEBUG
    print_baseline_pkt();
    write_pcap(base_pkt, baseline_pkt_len);
    #endif
}

void cea_stream::print_baseline_pkt() {
    ostringstream buf("");
    buf.setf(ios::hex, ios::basefield);
    buf.setf(ios_base::left);
    buf << endl;
    buf << cea_formatted_hdr("Baseline Packet");
    
    for (uint32_t idx=0; idx<baseline_pkt_len; idx++) {
        buf << setw(2) << right << setfill('0')<< hex << (uint16_t) base_pkt[idx] << " ";
        if (idx%8==7) buf << " ";
        if (idx%16==15) buf  << "(" << dec << (idx+1) << ")" << endl;
    }
    buf << endl << endl;

    cealog << buf.str();
}

bool cea_stream::is_touched(cea_field_id fid) {
    return fields[fid].touched;
}

uint32_t cea_stream::is_merge(cea_field_id fid) {
    if (fields[fid].merge != 0)
        return fields[fid].merge;
    else 
        return 0;
}

uint32_t cea_stream::value_of(cea_field_id fid) {
    return fields[fid].value;
}

// algorithm to organize the pkt fields
void cea_stream::organize_fields() {
    CEA_STREAM_DBG_CALL_SIGNATURE;

    fseq.insert(fseq.begin(),
        htof[MAC].begin(),
        htof[MAC].end());

    // TODO multiple vlan tags
    if (is_touched(VLAN_Tags)) {
        fseq.insert(fseq.end(), htof[VLAN].begin(), htof[VLAN].end());
    }

    if (value_of(PKT_Type) == ETH_V2) {
        fseq.push_back(MAC_Ether_Type);
    } else {
        fseq.push_back(MAC_Len);
    }

    if (value_of(PKT_Type) == ETH_LLC) {
        fseq.insert(fseq.end(), htof[LLC].begin(), htof[LLC].end());
    }

    if (value_of(PKT_Type) == ETH_SNAP) {
        fseq.insert(fseq.end(), htof[LLC].begin(), htof[LLC].end());
        fseq.insert(fseq.end(), htof[SNAP].begin(), htof[SNAP].end());
    }

    // TODO multiple mpls labels
    if (is_touched(MPLS_Labels)) {
        fseq.insert(fseq.end(), htof[MPLS].begin(), htof[MPLS].end());
    }

    fseq.insert(fseq.end(), 
        htof[(cea_hdr_type)value_of(Network_Hdr)].begin(), 
        htof[(cea_hdr_type)value_of(Network_Hdr)].end());

    fseq.insert(fseq.end(), 
        htof[(cea_hdr_type)value_of(Transport_Hdr)].begin(), 
        htof[(cea_hdr_type)value_of(Transport_Hdr)].end());

    // TODO add paddings if any

    #ifdef CEA_DEBUG
    uint32_t cntr=0;
    for (auto i : fseq) {
        CEA_MSG("(%s) Fn:%s: fseq: %-20s (%d)",
            stream_name.c_str(), __FUNCTION__, to_str(i).c_str(), cntr);
        cntr++;
    }
    #endif
}

void cea_stream::trim_static_fields() {
    CEA_STREAM_DBG_CALL_SIGNATURE;

    for(auto i: fseq) {
        if (is_touched(i)) {
            cseq.push_back(i);
        }
    }

    #ifdef CEA_DEBUG
    CEA_DBG("(%s) Fn:%s: Total Nof Fields: %d", stream_name.c_str(), __FUNCTION__, fseq.size());
    CEA_MSG("(%s) Fn:%s: Total Nof Consolidated Fields: %d", stream_name.c_str(), __FUNCTION__, cseq.size());
    uint32_t cntr=0;
    for (auto i : cseq) {
        CEA_MSG("(%s) Fn:%s: fseq: %-20s (%d)", 
            stream_name.c_str(), __FUNCTION__, to_str((cea_field_id)i).c_str(), cntr);
        cntr++;
    }
    #endif
}

// constructor
cea_stream::cea_stream(string name) {
    stream_id = cea::stream_id;
    cea::stream_id++;
    stream_name = name + ":" + to_string(stream_id);
    reset();
    // TODO: 1024 is just for testing
    //       the size of the base pkt should be calculated by
    //       summming the length of all the fields and payload
    //       properties
    base_pkt = new unsigned char[1024];
}

// copy constructor
cea_stream::cea_stream (const cea_stream &rhs) {
    cea_stream::do_copy(&rhs);
}

// assign operator overload
cea_stream& cea_stream::operator = (cea_stream &rhs) {
   if (this != &rhs) {
      do_copy(&rhs);
   }
   return *this;
}

// print operator overload
ostream& operator << (ostream &os, const cea_stream &f) {
    os << f.describe();
    return os;
}

// user api to set fields of the stream
void cea_stream::set(uint32_t id, uint64_t value) {
    fields[id].value = value;
    fields[id].touched = true;
}

// user api to set fields of the stream
void cea_stream::set(uint32_t id, cea_field_generation_type spec) {
    fields[id].touched = true;
}

void cea_stream::set(uint32_t id, cea_field_generation_type mspec, cea_field_generation_spec vspec) {
    fields[id].touched = true;
}

void cea_stream::add(uint32_t id) {
    fields[id].touched = true;
}

void cea_stream::do_copy (const cea_stream *rhs) {
    CEA_DBG("Stream CC Called");
}

void cea_stream::reset() {
    memcpy(&fields, &flds, (sizeof(cea_field)*(cea::Num_Fields)));
}

#define CEA_FLDWIDTH 8
string cea_stream::describe() const {
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
        << setw(CEA_FLDWIDTH+6) << "Value"  
        << setw(CEA_FLDWIDTH)   << "Start"  
        << setw(CEA_FLDWIDTH)   << "Stop "  
        << setw(CEA_FLDWIDTH)   << "Step "  
        << setw(CEA_FLDWIDTH)   << "Repeat"
        << "Name" << endl;

    for (uint32_t id = 0; id <cea::Network_Hdr; id++) {
        buf << setw(CEA_FLDWIDTH) << fields[id].touched 
            << setw(CEA_FLDWIDTH) << fields[id].merge    
            << setw(CEA_FLDWIDTH) << fields[id].mask     
            << setw(CEA_FLDWIDTH) << fields[id].id       
            << setw(CEA_FLDWIDTH) << fields[id].len      
            << setw(CEA_FLDWIDTH) << fields[id].offset    
            << setw(CEA_FLDWIDTH+2) << fields[id].gen_type 
            << setw(CEA_FLDWIDTH+6) << to_str((cea_pkt_type) fields[id].value)
            << setw(CEA_FLDWIDTH) << fields[id].start    
            << setw(CEA_FLDWIDTH) << fields[id].stop     
            << setw(CEA_FLDWIDTH) << fields[id].step     
            << setw(CEA_FLDWIDTH) << fields[id].repeat
            << fields[id].name;
            buf << endl;
    }

    for (uint32_t id = cea::Network_Hdr; id <cea::VLAN_Tags; id++) {
        buf << setw(CEA_FLDWIDTH) << fields[id].touched 
            << setw(CEA_FLDWIDTH) << fields[id].merge    
            << setw(CEA_FLDWIDTH) << fields[id].mask     
            << setw(CEA_FLDWIDTH) << fields[id].id       
            << setw(CEA_FLDWIDTH) << fields[id].len      
            << setw(CEA_FLDWIDTH) << fields[id].offset    
            << setw(CEA_FLDWIDTH+2) << fields[id].gen_type 
            << setw(CEA_FLDWIDTH+6) << to_str((cea_hdr_type) fields[id].value)
            << setw(CEA_FLDWIDTH) << fields[id].start    
            << setw(CEA_FLDWIDTH) << fields[id].stop     
            << setw(CEA_FLDWIDTH) << fields[id].step     
            << setw(CEA_FLDWIDTH) << fields[id].repeat
            << fields[id].name;
            buf << endl;
    }

    for (uint32_t id = VLAN_Tags; id <cea::Num_Fields; id++) {
        buf << setw(CEA_FLDWIDTH) << fields[id].touched 
            << setw(CEA_FLDWIDTH) << fields[id].merge    
            << setw(CEA_FLDWIDTH) << fields[id].mask     
            << setw(CEA_FLDWIDTH) << fields[id].id       
            << setw(CEA_FLDWIDTH) << fields[id].len      
            << setw(CEA_FLDWIDTH) << fields[id].offset    
            << setw(CEA_FLDWIDTH+2) << fields[id].gen_type 
            << setw(CEA_FLDWIDTH+6) << fields[id].value    
            << setw(CEA_FLDWIDTH) << fields[id].start    
            << setw(CEA_FLDWIDTH) << fields[id].stop     
            << setw(CEA_FLDWIDTH) << fields[id].step     
            << setw(CEA_FLDWIDTH) << fields[id].repeat
            << fields[id].name;
            buf << endl;
    }
    return buf.str();
}

} // namespace
