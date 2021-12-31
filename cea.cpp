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

string rtrim(string s) {
    s.erase(s.find_last_not_of(" \t\n\r\f\v") + 1);
    return s;
}

string ltrim(string s) {
    s.erase(0, s.find_first_not_of(" \t\n\r\f\v"));
    return s;
}

string trim(string s) {
    return ltrim(rtrim(s));
}

string wordwrap(string msg) {
    stringstream wrapped_msg;
    uint32_t line_width = 100;
    string leading_spaces = "    ";
    for (uint32_t pos=0; pos<msg.length(); pos=pos+line_width) {
        wrapped_msg << leading_spaces
        << ltrim(msg.substr(pos, line_width)) << endl;
    }
    return wrapped_msg.str();
}

#define CEA_FORMATTED_HDR_LEN 80
string formatted_hdr(string s) {
    stringstream ss;
    ss.setf(ios_base::left);
    ss << string(3,'-') << "{ ";
    ss << s << " }" << string((CEA_FORMATTED_HDR_LEN-(7+s.length())),'-');
    return ss.str();
}

string getEnvVar(string const& key) {
    char const* val = getenv(key.c_str());
    return val == NULL ? string() : string(val);
}

string name() {
    return "CEA";
}

string to_str(display_type t) {
    string name;
    switch(t) {
        case HEX : { name = "HEX      "; break; }
        case DEC : { name = "DEC      "; break; }
        case STR : { name = "STRING   "; break; }
        default  : { name = "UNDEFINED"; break; }
    }
    return trim(name);
}

string to_str(msg_verbosity t) {
    string name;
    switch(t) {
        case LOW  : { name = "LOW  "; break; }
        case FULL : { name = "FULL "; break; }
        default   : { name = "UNSET"; break; }
    }
    return trim(name);
}

// string to_str(msg_type t) {
//     string name;
//     switch(t) {
//         case TEST  : { name = "TEST     "; break; }          
//         case CFG   : { name = "CFG      "; break; }        
//         case CMD   : { name = "CMD      "; break; }        
//         case ERR   : { name = "ERR      "; break; }        
//         case WARN  : { name = "WARN     "; break; }          
//         case EVENT : { name = "EVENT    "; break; }          
//         case TXN   : { name = "TXN      "; break; }        
//         default    : { name = "UNDEFINED"; break; }
//     }
//     return trim(name);
// }

string to_str(field_modifier t) {
    string name;
    switch(t) {
        case CEA_Fixed             : { name = "Fixed            "; break; }                                    
        case CEA_Random            : { name = "Random           "; break; }                                    
        case CEA_Random_in_Range   : { name = "Random_in_Range  "; break; }                                    
        case CEA_Increment         : { name = "Increment        "; break; }                                    
        case CEA_Decrement         : { name = "Decrement        "; break; }                                    
        case CEA_Increment_Cycle   : { name = "Increment_Cycle  "; break; }                                    
        case CEA_Decrement_Cycle   : { name = "Decrement_Cycle  "; break; }                                   
        case CEA_Incr_Byte         : { name = "Incr_Byte        "; break; }                                    
        case CEA_Incr_Word         : { name = "Incr_Word        "; break; }                                      
        case CEA_Decr_Byte         : { name = "Decr_Byte        "; break; }                                       
        case CEA_Decr_Word         : { name = "Decr_Word        "; break; }                                        
        case CEA_Repeat_Pattern    : { name = "Repeat_Pattern   "; break; }                                         
        case CEA_Fixed_Pattern     : { name = "Fixed_Pattern    "; break; }                                          
        case CEA_Continuous_Pkts   : { name = "Continuous_Pkts  "; break; }                                  
        case CEA_Continuous_Burst  : { name = "Continuous_Burst "; break; }                                   
        case CEA_Stop_After_Stream : { name = "Stop_After_Stream"; break; }                                    
        case CEA_Goto_Next_Stream  : { name = "Goto_Next_Stream "; break; }                                   
        case CEA_Ipg               : { name = "Ipg              "; break; }                      
        case CEA_Percentage        : { name = "Percentage       "; break; }                             
        case CEA_Pkts_Per_Sec      : { name = "Pkts_Per_Sec     "; break; }                               
        case CEA_Bit_Rate          : { name = "Bit_Rate         "; break; }                           
        default                : { name = "UNDEFINED        "; break; }
    }
    return trim(name);
}

string to_str(field_id t) {
    string name;
    switch(t) {
        case CEA_MAC_Preamble            : { name = "MAC_Preamble           "; break; }                         
        case CEA_MAC_Dest_Addr           : { name = "MAC_Dest_Addr          "; break; }                          
        case CEA_MAC_Src_Addr            : { name = "MAC_Src_Addr           "; break; }                         
        case CEA_MAC_Len                 : { name = "MAC_Len                "; break; }                    
        case CEA_MAC_Ether_Type          : { name = "MAC_Ether_Type         "; break; }                           
        case CEA_MAC_Fcs                 : { name = "MAC_Fcs                "; break; }                    
        case CEA_IPv4_Version            : { name = "IPv4_Version           "; break; }                         
        case CEA_IPv4_IHL                : { name = "IPv4_IHL               "; break; }                     
        case CEA_IPv4_Tos                : { name = "IPv4_Tos               "; break; }                     
        case CEA_IPv4_Total_Len          : { name = "IPv4_Total_Len         "; break; }                           
        case CEA_IPv4_Id                 : { name = "IPv4_Id                "; break; }                    
        case CEA_IPv4_Flags              : { name = "IPv4_Flags             "; break; }                       
        case CEA_IPv4_Frag_Offset        : { name = "IPv4_Frag_Offset       "; break; }                             
        case CEA_IPv4_TTL                : { name = "IPv4_TTL               "; break; }                     
        case CEA_IPv4_Protocol           : { name = "IPv4_Protocol          "; break; }                          
        case CEA_IPv4_Hdr_Csum           : { name = "IPv4_Hdr_Csum          "; break; }                          
        case CEA_IPv4_Src_Addr           : { name = "IPv4_Src_Addr          "; break; }                          
        case CEA_IPv4_Dest_Addr          : { name = "IPv4_Dest_Addr         "; break; }                           
        case CEA_IPv4_Opts               : { name = "IPv4_Opts              "; break; }                      
        case CEA_IPv4_Pad                : { name = "IPv4_Pad               "; break; }                     
        case CEA_UDP_Src_Port            : { name = "UDP_Src_Port           "; break; }                         
        case CEA_UDP_Dest_Port           : { name = "UDP_Dest_Port          "; break; }                          
        case CEA_UDP_len                 : { name = "UDP_len                "; break; }                    
        case CEA_UDP_Csum                : { name = "UDP_Csum               "; break; }                     
        case CEA_STREAM_Type             : { name = "STREAM_Type            "; break; }                        
        case CEA_STREAM_Pkts_Per_Burst   : { name = "STREAM_Pkts_Per_Burst  "; break; }                                  
        case CEA_STREAM_Burst_Per_Stream : { name = "STREAM_Burst_Per_Stream"; break; }                                    
        case CEA_STREAM_Inter_Burst_Gap  : { name = "STREAM_Inter_Burst_Gap "; break; }                                   
        case CEA_STREAM_Inter_Stream_Gap : { name = "STREAM_Inter_Stream_Gap"; break; }                                    
        case CEA_STREAM_Start_Delay      : { name = "STREAM_Start_Delay     "; break; }                               
        case CEA_STREAM_Rate_Type        : { name = "STREAM_Rate_Type       "; break; }                             
        case CEA_STREAM_rate             : { name = "STREAM_rate            "; break; }                        
        case CEA_STREAM_Ipg              : { name = "STREAM_Ipg             "; break; }                       
        case CEA_STREAM_Percentage       : { name = "STREAM_Percentage      "; break; }                              
        case CEA_STREAM_PktsPerSec       : { name = "STREAM_PktsPerSec      "; break; }                              
        case CEA_STREAM_BitRate          : { name = "STREAM_BitRate         "; break; }                           
        case CEA_PAYLOAD_Type            : { name = "PAYLOAD_Type           "; break; }                         
        default : { name = "UNDEFINED"; break; }
    }
    return trim(name);
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

void cea_stream::do_copy (const cea_stream* rhs) {
    CEA_DBG("Stream CC Called");
}

cea_stream::cea_stream () {
}

cea_stream::cea_stream (const cea_stream& rhs) {
    cea_stream::do_copy(&rhs);
}

cea_stream& cea_stream::operator = (cea_stream& rhs) {
   if (this != &rhs) {
      do_copy(&rhs);
   }
   return *this;
}

ostream& operator << (ostream& os, const cea_stream& f) {
    os << f.describe();
    return os;
}

string cea_stream::describe() const {
    return "";
}

} // namesapce
