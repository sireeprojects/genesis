#include "cea_field.h"
#include <random>
#include <algorithm>
#include <sstream>
#include <regex>

namespace cea {

string msg_prefix = "cea";    

#define CEA_FORMATTED_HDR_LEN 80

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

string cea_formatted_hdr(string s) {
    stringstream ss;
    ss.setf(ios_base::left);
    ss << string(3,'-') << "{ ";
    ss << s << " }" << string((CEA_FORMATTED_HDR_LEN-(7+s.length())),'-');
    ss << endl;
    return ss.str();
}

#define CEA_ERR_MSG(msg) { \
    stringstream s; \
    s << msg; \
    cealog << endl << cea_formatted_hdr("Fatal Error"); \
    cealog << "(" << msg_prefix << "|" << string(__FUNCTION__) << ")" \
    << ": " <<  s.str() << endl; \
    cealog << string(CEA_FORMATTED_HDR_LEN, '-') << endl; \
}

enum cea_field_type {
    Integer,
    Pattern_PRE,
    Pattern_MAC,
    Pattern_IPv4,
    Pattern_IPv6
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

class cea_field::core {
public:
    // ctor
    core(cea_field_id id);

    // dtor
    ~core();

    // Quickly set a fixed integer value to a field
    void set(uint64_t value);

    // Quickly set a fixed string value to a field
    void set(string value);

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

cea_field::cea_field(cea_field_id id) {
    impl = make_unique<core>(id); 
}

cea_field::~cea_field() = default;

cea_field::core::core(cea_field_id id) {
    field_id = id;
    field = get_field(mtable, id);
}

cea_field::core::~core() = default;

void cea_field::set(uint64_t value) {
    impl->set(value);
}

void cea_field::set(string value) {
    impl->set(value);
}

void cea_field::set(cea_field_genspec spec) {
    impl->set(spec);
}

void cea_field::core::set(uint64_t value) {
    if (field.defaults.type != Integer) {
        CEA_ERR_MSG("The field "
        << cea_trim(field.defaults.name) << " accepts only string patterns");
        abort();
    }
    field.gspec.nmr.value = value;
    field.gspec.gen_type = Fixed_Value;
}

void cea_field::core::set(string value) {
    if (field.defaults.type == Integer) {
        CEA_ERR_MSG("The field "
        << cea_trim(field.defaults.name) << " accepts only integer values");
        abort();
    }
    field.gspec.str.value = value;
    field.gspec.gen_type = Fixed_Value;
}

void cea_field::core::set(cea_field_genspec spec) {
    field.gspec = spec;
}

}
