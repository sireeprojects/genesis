#include "cea.h"

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

#define CEA_DBG_CALL_SIGNATURE CEA_DBG("(%s) Fn:%s: Invoked", name().c_str(), __FUNCTION__);

namespace cea {

cea_field flds[] = {
// Toc Mrg Mask Id                        Len Offset Modifier Val Start Stop Step Rpt Name
{  0,  0,  0,   PKT_Type                 ,8,  0,     Fixed,   Ethernet_V2,  0,    0,   0,   0,  "PKT_Type               "},                                     
{  0,  0,  0,   PKT_Network_Hdr          ,8,  0,     Fixed,   IPv4,         0,    0,   0,   0,  "PKT_Network_Hdr        "},
{  0,  0,  0,   PKT_Transport_Hdr        ,8,  0,     Fixed,   UDP,          0,    0,   0,   0,  "PKT_Transport_Hdr      "},
{  0,  0,  0,   PKT_VLAN_Tags            ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "PKT_VLAN_Tags          "},
{  0,  0,  0,   PKT_MPLS_Labels          ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "PKT_MPLS_Labels        "},
{  0,  0,  0,   MAC_Preamble             ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "MAC_Preamble           "},
{  0,  0,  0,   MAC_Dest_Addr            ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "MAC_Dest_Addr          "},
{  0,  0,  0,   MAC_Src_Addr             ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "MAC_Src_Addr           "},
{  0,  0,  0,   MAC_Len                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "MAC_Len                "},
{  0,  0,  0,   MAC_Ether_Type           ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "MAC_Ether_Type         "},
{  0,  0,  0,   MAC_Fcs                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "MAC_Fcs                "},
{  0,  0,  0,   VLAN_Tpi                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "VLAN_Tpi               "},
{  0,  0,  0,   VLAN_Tci_Pcp             ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "VLAN_Tci_Pcp           "},
{  0,  0,  0,   VLAN_Tci_Cfi             ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "VLAN_Tci_Cfi           "},
{  0,  0,  0,   VLAN_Vid                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "VLAN_Vid               "},
{  0,  0,  0,   LLC_Dsap                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "LLC_Dsap               "},
{  0,  0,  0,   LLC_Ssap                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "LLC_Ssap               "},
{  0,  0,  0,   LLC_Control              ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "LLC_Control            "},
{  0,  0,  0,   SNAP_Oui                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "SNAP_Oui               "},
{  0,  0,  0,   SNAP_Pid                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "SNAP_Pid               "},
{  0,  0,  0,   MPLS_Label               ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "MPLS_Label             "},
{  0,  0,  0,   MPLS_Cos                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "MPLS_Cos               "},
{  0,  0,  0,   MPLS_Stack               ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "MPLS_Stack             "},
{  0,  0,  0,   MPLS_Ttl                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "MPLS_Ttl               "},
{  0,  0,  0,   IPv4_Version             ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Version           "},
{  0,  0,  0,   IPv4_IHL                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_IHL               "},
{  0,  0,  0,   IPv4_Tos                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Tos               "},
{  0,  0,  0,   IPv4_Total_Len           ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Total_Len         "},
{  0,  0,  0,   IPv4_Id                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Id                "},
{  0,  0,  0,   IPv4_Flags               ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Flags             "},
{  0,  0,  0,   IPv4_Frag_Offset         ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Frag_Offset       "},
{  0,  0,  0,   IPv4_TTL                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_TTL               "},
{  0,  0,  0,   IPv4_Protocol            ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Protocol          "},
{  0,  0,  0,   IPv4_Hdr_Csum            ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Hdr_Csum          "},
{  0,  0,  0,   IPv4_Src_Addr            ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Src_Addr          "},
{  0,  0,  0,   IPv4_Dest_Addr           ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Dest_Addr         "},
{  0,  0,  0,   IPv4_Opts                ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Opts              "},
{  0,  0,  0,   IPv4_Pad                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv4_Pad               "},
{  0,  0,  0,   IPv6_Version             ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv6_Version           "},
{  0,  0,  0,   IPv6_Traffic_Class       ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv6_Traffic_Class     "},
{  0,  0,  0,   IPv6_Flow_Label          ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv6_Flow_Label        "},
{  0,  0,  0,   IPv6_Payload_Len         ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv6_Payload_Len       "},
{  0,  0,  0,   IPv6_Next_Hdr            ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv6_Next_Hdr          "},
{  0,  0,  0,   IPv6_Hop_Limit           ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv6_Hop_Limit         "},
{  0,  0,  0,   IPv6_Src_Addr            ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv6_Src_Addr          "},
{  0,  0,  0,   IPv6_Dest_Addr           ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "IPv6_Dest_Addr         "},
{  0,  0,  0,   TCP_Src_Port             ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_Src_Port           "},
{  0,  0,  0,   TCP_Dest_Port            ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_Dest_Port          "},
{  0,  0,  0,   TCP_Seq_Num              ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_Seq_Num            "},
{  0,  0,  0,   TCP_Ack_Num              ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_Ack_Num            "},
{  0,  0,  0,   TCP_Data_Offset          ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_Data_Offset        "},
{  0,  0,  0,   TCP_Reserved             ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_Reserved           "},
{  0,  0,  0,   TCP_URG                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_URG                "},
{  0,  0,  0,   TCP_ACK                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_ACK                "},
{  0,  0,  0,   TCP_PSH                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_PSH                "},
{  0,  0,  0,   TCP_RST                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_RST                "},
{  0,  0,  0,   TCP_SYN                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_SYN                "},
{  0,  0,  0,   TCP_FIN                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_FIN                "},
{  0,  0,  0,   TCP_Window               ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_Window             "},
{  0,  0,  0,   TCP_Csum                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_Csum               "},
{  0,  0,  0,   TCP_UrgPtr               ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_UrgPtr             "},
{  0,  0,  0,   TCP_Opts                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_Opts               "},
{  0,  0,  0,   TCP_Pad                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "TCP_Pad                "},
{  0,  0,  0,   UDP_Src_Port             ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "UDP_Src_Port           "},
{  0,  0,  0,   UDP_Dest_Port            ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "UDP_Dest_Port          "},
{  0,  0,  0,   UDP_len                  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "UDP_len                "},
{  0,  0,  0,   UDP_Csum                 ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "UDP_Csum               "},
{  0,  0,  0,   ARP_Hw_Type              ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "ARP_Hw_Type            "},
{  0,  0,  0,   ARP_Proto_Type           ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "ARP_Proto_Type         "},
{  0,  0,  0,   ARP_Hw_Len               ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "ARP_Hw_Len             "},
{  0,  0,  0,   ARP_Proto_Len            ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "ARP_Proto_Len          "},
{  0,  0,  0,   ARP_Opcode               ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "ARP_Opcode             "},
{  0,  0,  0,   ARP_Sender_Hw_Addr       ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "ARP_Sender_Hw_Addr     "},
{  0,  0,  0,   ARP_Sender_Proto_addr    ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "ARP_Sender_Proto_addr  "},
{  0,  0,  0,   ARP_Target_Hw_Addr       ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "ARP_Target_Hw_Addr     "},
{  0,  0,  0,   ARP_Target_Proto_Addr    ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "ARP_Target_Proto_Addr  "},
{  0,  0,  0,   STREAM_Type              ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_Type            "},
{  0,  0,  0,   STREAM_Pkts_Per_Burst    ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_Pkts_Per_Burst  "},
{  0,  0,  0,   STREAM_Burst_Per_Stream  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_Burst_Per_Stream"},
{  0,  0,  0,   STREAM_Inter_Burst_Gap   ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_Inter_Burst_Gap "},
{  0,  0,  0,   STREAM_Inter_Stream_Gap  ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_Inter_Stream_Gap"},
{  0,  0,  0,   STREAM_Start_Delay       ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_Start_Delay     "},
{  0,  0,  0,   STREAM_Rate_Type         ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_Rate_Type       "},
{  0,  0,  0,   STREAM_rate              ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_rate            "},
{  0,  0,  0,   STREAM_Ipg               ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_Ipg             "},
{  0,  0,  0,   STREAM_Percentage        ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_Percentage      "},
{  0,  0,  0,   STREAM_PktsPerSec        ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_PktsPerSec      "},
{  0,  0,  0,   STREAM_BitRate           ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "STREAM_BitRate         "},
{  0,  0,  0,   PAYLOAD_Type             ,8,  0,     Fixed,   0,            0,    0,   0,   0,  "PAYLOAD_Type           "}
};

map<cea_pkt_hdr_type, vector<cea_field_id> >htof = {
    {MAC,   {
            MAC_Preamble,
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
            TCP_URG,
            TCP_ACK,
            TCP_PSH,
            TCP_RST,
            TCP_SYN,
            TCP_FIN,
            TCP_Window,
            TCP_Csum,
            TCP_UrgPtr,
            TCP_Opts,
            TCP_Pad
            }},
    {UDP, { UDP_Src_Port,
            UDP_Dest_Port,
            UDP_len,
            UDP_Csum,
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
            cout << "Error creating logfile. Aborting..." << endl;
            exit(1);
        }
    }
    ~cea_init() { logfile.close(); }
};
cea_init init;

struct CEA_PACKED pcap_file_hdr {
    unsigned int magic           : 32;
    unsigned short version_major : 16;
    unsigned short version_minor : 16;
    unsigned int thiszone        : 32;
    unsigned int sigfigs         : 32;
    unsigned int snaplen         : 32;
    unsigned int linktype        : 32;
};

struct CEA_PACKED pcap_pkt_hdr {
    unsigned int tv_sec  : 32;
    unsigned int tv_usec : 32;
    unsigned int caplen  : 32;
    unsigned int len     : 32;
};

void write_pcap(char *pkt, uint32_t len) {
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
    ph.caplen = 64;
    ph.len = len;

    char buf[16384];
    memcpy(buf, (char*)&fh, sizeof(pcap_file_hdr));
    memcpy(buf+sizeof(pcap_file_hdr), (char*)&ph, sizeof(pcap_pkt_hdr));
    memcpy(buf+(sizeof(pcap_file_hdr)+sizeof(pcap_file_hdr)), pkt, len);

    ofstream pcapfile;
    pcapfile.open("run.pcap", ofstream::app);
    pcapfile << buf;
    pcapfile.close();
}

string cea_rtrim(string s) {
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    return s;
}

string cea_ltrim(string s) {
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
    return s;
}

string cea_trim(string s) {
    return cea_ltrim(cea_rtrim(s));
}

string cea_wordwrap(string msg) {
    stringstream wrapped_msg;
    uint32_t line_width = 100;
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

string cea_getenv(string &key) {
    char *val = getenv(key.c_str());
    return (val==NULL) ? string() : string(val);
}

typedef enum {
    KIS1024 = 1024,
    KIS1000 = 1000
} cea_readable_type;

string cea_readable_fs(double size, cea_readable_type type) {
    int i = 0;
    ostringstream buf("");
    const char *units[] = {"B", "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
    int divider = type;

    while (size > (divider-1)) {
      size /= divider;
      i++;
    }
    buf << fixed << setprecision(2) << size << " " << units[i] << endl; 
    return buf.str();
}

string to_str(cea_pkt_type t) {
    string name;
    switch(t) {
        case Ethernet_V2   : { name = "Ethernet_V2  "; break; }
        case Ethernet_LLC  : { name = "Ethernet_LLC "; break; }
        case Ethernet_SNAP : { name = "Ethernet_SNAP"; break; }
        default            : { name = "undefined    "; break; }
    }
    return cea_trim(name);
}

string to_str(cea_pkt_hdr_type t) {
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

string to_str(cea_field_print_type t) {
    string name;
    switch(t) {
        case HEX : { name = "HEX      "; break; }
        case DEC : { name = "DEC      "; break; }
        case STR : { name = "STRING   "; break; }
        default  : { name = "UNDEFINED"; break; }
    }
    return cea_trim(name);
}

string to_str(cea_msg_verbosity t) {
    string name;
    switch(t) {
        case LOW  : { name = "LOW  "; break; }
        case FULL : { name = "FULL "; break; }
        default   : { name = "UNSET"; break; }
    }
    return cea_trim(name);
}

string to_str(cea_field_modifier t) {
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

string to_str(cea_field_id t) {
    string name;
    switch(t) {
        case PKT_Type                : { name = "PKT_Type               "; break; }
        case PKT_Network_Hdr         : { name = "PKT_Network_Hdr        "; break; }
        case PKT_Transport_Hdr       : { name = "PKT_Transport_Hdr      "; break; }
        case PKT_VLAN_Tags           : { name = "PKT_VLAN_Tags          "; break; }
        case PKT_MPLS_Labels         : { name = "PKT_MPLS_Labels        "; break; }
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
        case TCP_URG                 : { name = "TCP_URG                "; break; }
        case TCP_ACK                 : { name = "TCP_ACK                "; break; }
        case TCP_PSH                 : { name = "TCP_PSH                "; break; }
        case TCP_RST                 : { name = "TCP_RST                "; break; }
        case TCP_SYN                 : { name = "TCP_SYN                "; break; }
        case TCP_FIN                 : { name = "TCP_FIN                "; break; }
        case TCP_Window              : { name = "TCP_Window             "; break; }
        case TCP_Csum                : { name = "TCP_Csum               "; break; }
        case TCP_UrgPtr              : { name = "TCP_UrgPtr             "; break; }
        case TCP_Opts                : { name = "TCP_Opts               "; break; }
        case TCP_Pad                 : { name = "TCP_Pad                "; break; }
        case UDP_Src_Port            : { name = "UDP_Src_Port           "; break; }
        case UDP_Dest_Port           : { name = "UDP_Dest_Port          "; break; }
        case UDP_len                 : { name = "UDP_len                "; break; }
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
        case STREAM_Type             : { name = "STREAM_Type            "; break; }
        case STREAM_Pkts_Per_Burst   : { name = "STREAM_Pkts_Per_Burst  "; break; }
        case STREAM_Burst_Per_Stream : { name = "STREAM_Burst_Per_Stream"; break; }
        case STREAM_Inter_Burst_Gap  : { name = "STREAM_Inter_Burst_Gap "; break; }
        case STREAM_Inter_Stream_Gap : { name = "STREAM_Inter_Stream_Gap"; break; }
        case STREAM_Start_Delay      : { name = "STREAM_Start_Delay     "; break; }
        case STREAM_Rate_Type        : { name = "STREAM_Rate_Type       "; break; }
        case STREAM_rate             : { name = "STREAM_rate            "; break; }
        case STREAM_Ipg              : { name = "STREAM_Ipg             "; break; }
        case STREAM_Percentage       : { name = "STREAM_Percentage      "; break; }
        case STREAM_PktsPerSec       : { name = "STREAM_PktsPerSec      "; break; }
        case STREAM_BitRate          : { name = "STREAM_BitRate         "; break; }
        case PAYLOAD_Type            : { name = "PAYLOAD_Type           "; break; }
        default                      : { name = "undefined              "; break; }
    }
    return cea_trim(name);
}

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
            if ((*it)->pid == pxy->pid) {
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

void cea_manager::testfn(cea_proxy *p) {
}

void cea_manager::add_cmd(cea_stream *stm, cea_proxy *pxy) {
    CEA_DBG("%s called", __FUNCTION__);
    add_stream(stm, pxy);
}

void cea_manager::exec_cmd(cea_stream *stm, cea_proxy *pxy) {
    CEA_DBG("%s called", __FUNCTION__);
}

// ctor
cea_proxy::cea_proxy(string name) {
    this->pid = cea::pid;
    cea::pid++;
    this->pname = name;
    CEA_DBG("(%s) Fn:%s: ProxyID: %d", name.c_str(), __FUNCTION__, pid);
}

void cea_proxy::add_stream(cea_stream *stm) {
    streamq.push_back(stm);
}

void cea_proxy::add_cmd(cea_stream *stm) {
    streamq.push_back(stm);
}

void cea_proxy::exec_cmd(cea_stream *stm) {
}

void cea_proxy::testfn() {
    start_worker();
    join_threads();
}

void cea_proxy::join_threads() {
    w.join();
}

string cea_proxy::name() {
    return pname;
}

void cea_proxy::start_worker() {
    w = thread (&cea_proxy::worker, this);
    char name[16];
    sprintf(name, "worker_%d", port_num);
    pthread_setname_np(w.native_handle(), name);
}

void cea_proxy::read() {
    CEA_DBG_CALL_SIGNATURE;
    cur_stream = streamq[0];
    // cealog << *cur_stream;
}

void cea_proxy::worker() {
    read();
    generate_field_sequence();
    consolidate_fields();
    set_gen_vars();
    generate();
}

bool cea_stream::is_touched(cea_field_id fid) {
    return fields[fid].touched;
}

uint32_t cea_stream::value_of(cea_field_id fid) {
    return fields[fid].value;
}

// algorithm to organize the pkt fields
void cea_proxy::generate_field_sequence() {
    CEA_DBG_CALL_SIGNATURE;

    fseq.insert(fseq.begin(), htof[MAC].begin(), htof[MAC].end());

    // TODO multiple vlan tags
    if (cur_stream->is_touched(PKT_VLAN_Tags)) {
        fseq.insert(fseq.end(), htof[VLAN].begin(), htof[VLAN].end());
    }

    if (cur_stream->value_of(PKT_Type) == (cea_pkt_type) Ethernet_V2) {
        fseq.push_back((cea_field_id) MAC_Ether_Type);
    } else {
        fseq.push_back((cea_field_id) MAC_Len);
    }

    if (cur_stream->value_of(PKT_Type) == (cea_pkt_type) Ethernet_LLC) {
        fseq.insert(fseq.end(), htof[LLC].begin(), htof[LLC].end());
    }

    if (cur_stream->value_of(PKT_Type) == (cea_pkt_type) Ethernet_SNAP) {
        fseq.insert(fseq.end(), htof[LLC].begin(), htof[LLC].end());
        fseq.insert(fseq.end(), htof[SNAP].begin(), htof[SNAP].end());
    }

    // TODO multiple mpls labels
    if (cur_stream->is_touched(PKT_MPLS_Labels)) {
        fseq.insert(fseq.end(), htof[MPLS].begin(), htof[MPLS].end());
    }

    fseq.insert(fseq.end(), 
        htof[(cea_pkt_hdr_type)cur_stream->value_of(PKT_Network_Hdr)].begin(), 
        htof[(cea_pkt_hdr_type)cur_stream->value_of(PKT_Network_Hdr)].end());

    fseq.insert(fseq.end(), 
        htof[(cea_pkt_hdr_type)cur_stream->value_of(PKT_Transport_Hdr)].begin(), 
        htof[(cea_pkt_hdr_type)cur_stream->value_of(PKT_Transport_Hdr)].end());

    for (auto i : fseq) {
        CEA_DBG("(%s) Fn:%s: fseq: %s (%d)", name().c_str(), __FUNCTION__, to_str((cea_field_id)i).c_str(), i);
    }

}

void cea_proxy::consolidate_fields() {
    CEA_DBG_CALL_SIGNATURE;
    CEA_DBG("(%s) Fn:%s: Total Nof Fields: %d", name().c_str(), __FUNCTION__, fseq.size());

    for(auto i: fseq) {
        if (cur_stream->is_touched((cea_field_id)i)) {
            consolidated_fseq.push_back(i);
        }
    }
    CEA_DBG("(%s) Fn:%s: Total Nof Consolidated Fields: %d", name().c_str(), __FUNCTION__, consolidated_fseq.size());
}

void cea_proxy::set_gen_vars() {
    CEA_DBG_CALL_SIGNATURE;
}

void cea_proxy::generate() {
    CEA_DBG_CALL_SIGNATURE;
    // write to fbuf
    // *(addr + i) = (char)i;
    // read from fbuf
    // if (*(addr + i) != (char)i)
}

void cea_proxy::create_frm_buffer() {
    fbuf = mmap(ADDR, LENGTH, PROTECTION, FLAGS, -1, 0);
    if (fbuf == MAP_FAILED) {
        CEA_MSG("Error: Memory map failed in __FUNCTION__");
        exit(1);
    }
}

void cea_proxy::release_frm_buffer() {
    if (munmap(fbuf, LENGTH)) {
        CEA_MSG("Error: Memory unmap failed in __FUNCTION__");
        exit(1);
    }
}

//--------
// Stream
//--------

// constructor
cea_stream::cea_stream() {
    reset();
}

// copy constructor
cea_stream::cea_stream (const cea_stream& rhs) {
    cea_stream::do_copy(&rhs);
}

// assign operator overload
cea_stream& cea_stream::operator = (cea_stream& rhs) {
   if (this != &rhs) {
      do_copy(&rhs);
   }
   return *this;
}

// print operator overload
ostream& operator << (ostream& os, const cea_stream& f) {
    os << f.describe();
    return os;
}

// user api to set fields of the stream
void cea_stream::set(uint32_t id, uint64_t value) {
    fields[id].value = value;
    fields[id].touched = true;
}

// user api to set fields of the stream
void cea_stream::set(uint32_t id, cea_field_modifier spec) {
    fields[id].touched = true;
}

void cea_stream::set(uint32_t id, cea_field_modifier mspec, cea_value_spec vspec) {
    fields[id].touched = true;
}

void cea_stream::add(uint32_t id) {
    fields[id].touched = true;
}

char* cea_stream::pack() {
    return NULL;
}

void cea_stream::unpack(char *data) {
}

void cea_stream::do_copy (const cea_stream* rhs) {
    CEA_DBG("Stream CC Called");
}

void cea_stream::testfn() {
}

void cea_stream::reset() {
    memcpy(&fields, &flds, sizeof(cea_field)*cea::NumFields);
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

    for (uint32_t id = 0; id <cea::PKT_Network_Hdr; id++) {
        buf << setw(CEA_FLDWIDTH) << fields[id].touched 
            << setw(CEA_FLDWIDTH) << fields[id].merge    
            << setw(CEA_FLDWIDTH) << fields[id].mask     
            << setw(CEA_FLDWIDTH) << fields[id].id       
            << setw(CEA_FLDWIDTH) << fields[id].len      
            << setw(CEA_FLDWIDTH) << fields[id].offset    
            << setw(CEA_FLDWIDTH+2) << fields[id].modifier 
            << setw(CEA_FLDWIDTH+6) << to_str((cea_pkt_type) fields[id].value)
            << setw(CEA_FLDWIDTH) << fields[id].start    
            << setw(CEA_FLDWIDTH) << fields[id].stop     
            << setw(CEA_FLDWIDTH) << fields[id].step     
            << setw(CEA_FLDWIDTH) << fields[id].repeat
            << fields[id].name;
            buf << endl;
    }

    for (uint32_t id = cea::PKT_Network_Hdr; id <cea::PKT_VLAN_Tags; id++) {
        buf << setw(CEA_FLDWIDTH) << fields[id].touched 
            << setw(CEA_FLDWIDTH) << fields[id].merge    
            << setw(CEA_FLDWIDTH) << fields[id].mask     
            << setw(CEA_FLDWIDTH) << fields[id].id       
            << setw(CEA_FLDWIDTH) << fields[id].len      
            << setw(CEA_FLDWIDTH) << fields[id].offset    
            << setw(CEA_FLDWIDTH+2) << fields[id].modifier 
            << setw(CEA_FLDWIDTH+6) << to_str((cea_pkt_hdr_type) fields[id].value)
            << setw(CEA_FLDWIDTH) << fields[id].start    
            << setw(CEA_FLDWIDTH) << fields[id].stop     
            << setw(CEA_FLDWIDTH) << fields[id].step     
            << setw(CEA_FLDWIDTH) << fields[id].repeat
            << fields[id].name;
            buf << endl;
    }

    for (uint32_t id = PKT_VLAN_Tags; id <cea::NumFields; id++) {
        buf << setw(CEA_FLDWIDTH) << fields[id].touched 
            << setw(CEA_FLDWIDTH) << fields[id].merge    
            << setw(CEA_FLDWIDTH) << fields[id].mask     
            << setw(CEA_FLDWIDTH) << fields[id].id       
            << setw(CEA_FLDWIDTH) << fields[id].len      
            << setw(CEA_FLDWIDTH) << fields[id].offset    
            << setw(CEA_FLDWIDTH+2) << fields[id].modifier 
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

} // namesapce
