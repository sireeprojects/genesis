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

namespace cea {

cea_field flds[] = {
// Toc Mrg Mask Id                      Len Offset Modifier Val Start Stop Step Rpt Name
{  0,  0,  0,   MAC_Preamble           ,8,  0,     Fixed,   0,  0,    0,   0,   0,  "MAC_Preamble           "},
{  0,  0,  0,   MAC_Dest_Addr          ,6,  0,     Fixed,   0,  0,    0,   0,   0,  "MAC_Dest_Addr          "},
{  0,  0,  0,   MAC_Src_Addr           ,6,  0,     Fixed,   0,  0,    0,   0,   0,  "MAC_Src_Addr           "},
{  0,  0,  0,   MAC_Len                ,2,  0,     Fixed,   0,  0,    0,   0,   0,  "MAC_Len                "},
{  0,  0,  0,   MAC_Ether_Type         ,2,  0,     Fixed,   0,  0,    0,   0,   0,  "MAC_Ether_Type         "},
{  0,  0,  0,   MAC_Fcs                ,4,  0,     Fixed,   0,  0,    0,   0,   0,  "MAC_Fcs                "},
{  0,  0,  0,   IPv4_Version           ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Version           "},
{  0,  0,  0,   IPv4_IHL               ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_IHL               "},
{  0,  0,  0,   IPv4_Tos               ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Tos               "},
{  0,  0,  0,   IPv4_Total_Len         ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Total_Len         "},
{  0,  0,  0,   IPv4_Id                ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Id                "},
{  0,  0,  0,   IPv4_Flags             ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Flags             "},
{  0,  0,  0,   IPv4_Frag_Offset       ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Frag_Offset       "},
{  0,  0,  0,   IPv4_TTL               ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_TTL               "},
{  0,  0,  0,   IPv4_Protocol          ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Protocol          "},
{  0,  0,  0,   IPv4_Hdr_Csum          ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Hdr_Csum          "},
{  0,  0,  0,   IPv4_Src_Addr          ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Src_Addr          "},
{  0,  0,  0,   IPv4_Dest_Addr         ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Dest_Addr         "},
{  0,  0,  0,   IPv4_Opts              ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Opts              "},
{  0,  0,  0,   IPv4_Pad               ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "IPv4_Pad               "},
{  0,  0,  0,   UDP_Src_Port           ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "UDP_Src_Port           "},
{  0,  0,  0,   UDP_Dest_Port          ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "UDP_Dest_Port          "},
{  0,  0,  0,   UDP_len                ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "UDP_len                "},
{  0,  0,  0,   UDP_Csum               ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "UDP_Csum               "},
{  0,  0,  0,   STREAM_Type            ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_Type            "},
{  0,  0,  0,   STREAM_Pkts_Per_Burst  ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_Pkts_Per_Burst  "},
{  0,  0,  0,   STREAM_Burst_Per_Stream,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_Burst_Per_Stream"},
{  0,  0,  0,   STREAM_Inter_Burst_Gap ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_Inter_Burst_Gap "},
{  0,  0,  0,   STREAM_Inter_Stream_Gap,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_Inter_Stream_Gap"},
{  0,  0,  0,   STREAM_Start_Delay     ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_Start_Delay     "},
{  0,  0,  0,   STREAM_Rate_Type       ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_Rate_Type       "},
{  0,  0,  0,   STREAM_rate            ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_rate            "},
{  0,  0,  0,   STREAM_Ipg             ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_Ipg             "},
{  0,  0,  0,   STREAM_Percentage      ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_Percentage      "},
{  0,  0,  0,   STREAM_PktsPerSec      ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_PktsPerSec      "},
{  0,  0,  0,   STREAM_BitRate         ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "STREAM_BitRate         "},
{  0,  0,  0,   PAYLOAD_Type           ,0,  0,     Fixed,   0,  0,    0,   0,   0,  "PAYLOAD_Type           "}
};

map<cea_pkt_hdr_type, vector<cea_field_id> >htof = {
    {MAC, { MAC_Preamble,
            MAC_Dest_Addr,
            MAC_Src_Addr
            }},
    {VLAN,  {
            }},
    {MPLS,  {
            }},
    {LLC,   {
            }},
    {SNAP,  {
            }},
    {IPv4,{ IPv4_Version,
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
    {IPv6,  {}},
    {ARP,   {}},
    {TCP,   {}},
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
        case MAC_Preamble            : { name = "MAC_Preamble           "; break; }                         
        case MAC_Dest_Addr           : { name = "MAC_Dest_Addr          "; break; }                          
        case MAC_Src_Addr            : { name = "MAC_Src_Addr           "; break; }                         
        case MAC_Len                 : { name = "MAC_Len                "; break; }                    
        case MAC_Ether_Type          : { name = "MAC_Ether_Type         "; break; }                           
        case MAC_Fcs                 : { name = "MAC_Fcs                "; break; }                    
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
        case UDP_Src_Port            : { name = "UDP_Src_Port           "; break; }                         
        case UDP_Dest_Port           : { name = "UDP_Dest_Port          "; break; }                          
        case UDP_len                 : { name = "UDP_len                "; break; }                    
        case UDP_Csum                : { name = "UDP_Csum               "; break; }                     
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
        default : { name = "UNDEFINED"; break; }
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
    // TODO
}

cea_proxy::cea_proxy() {
    this->pid = cea::pid;
    cea::pid++;
    CEA_DBG("%s : Value of pid: %d", __FUNCTION__, pid);
}

void cea_proxy::add_stream(cea_stream *stm) {
    streamq.push_back(stm);
}

void cea_proxy::add_cmd(cea_stream *stm) {
    streamq.push_back(stm);
}

void cea_proxy::exec_cmd(cea_stream *stm) {
    // TODO
}

void cea_proxy::testfn(cea_stream *s) {
}

void cea_proxy::start_worker() {
    w = thread (&cea_proxy::worker, this);
    char name[16];
    sprintf(name, "worker_%d", port_num);
    pthread_setname_np(w.native_handle(), name);
    w.detach();
}

void cea_proxy::read() {
}

void cea_proxy::worker() {
    read();
    consolidate();
    set_gen_vars();
    generate();
}

void cea_proxy::consolidate() {
}

void cea_proxy::set_gen_vars() {
}

void cea_proxy::generate() {
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

    for (uint32_t id = 0; id <cea::NumFields; id++) {
        buf << setw(CEA_FLDWIDTH) << fields[id].touched 
            << setw(CEA_FLDWIDTH) << fields[id].merge    
            << setw(CEA_FLDWIDTH) << fields[id].mask     
            << setw(CEA_FLDWIDTH) << fields[id].id       
            << setw(CEA_FLDWIDTH) << fields[id].len      
            << setw(CEA_FLDWIDTH) << fields[id].ofset    
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
