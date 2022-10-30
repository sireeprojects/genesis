#include "cea.h"
using namespace cea;

int main() {
    // proxy instance
    cea_proxy *p = new cea_proxy();

    // stream
    cea_stream *s = new cea_stream();

    cea_field_generation_type lentype;
    cea_field_generation_spec lenspec;
    lentype =  Fixed;
    lenspec.value = 64;
    lenspec.start = 0;
    lenspec.stop = 0;
    lenspec.step = 0;
    lenspec.repeat = 0;

    cea_field_generation_type ptype;
    cea_field_generation_spec pspec;
    // ptype =  Fixed_Pattern;
    // ptype =  Incr_Byte;
    // ptype =  Incr_Word;
    // ptype =  Decr_Byte;
    ptype =  Decr_Word;
    pspec.value = 64;
    pspec.start = 0;
    pspec.stop = 10;
    pspec.step = 0;
    pspec.repeat = 0;

    s->set(FRAME_Len, lentype);
    s->set(PAYLOAD_Type, ptype);
    // s->set(FRAME_Len, 64);
    
    s->set(FRAME_Type, ETH_V2);
    s->set(Network_Hdr, IPv4);
    s->set(Transport_Hdr, TCP);
    s->set(STREAM_Pkts_Per_Burst, 10);
    s->set(STREAM_Crc_Enable, 1);

    // add stream to proxy queue
    p->add_stream(s);
    // start stream processing
    p->start();
    return 0;
}

// int main() {
//     // proxy instance
//     cea_proxy *p = new cea_proxy();
// 
//     // stream
//     cea_stream *s = new cea_stream();
// 
//     cea_field_generation_type lentype;
//     cea_field_generation_spec lenspec;
//     lentype =  Fixed;
//     lenspec.value = 64;
//     lenspec.start = 0;
//     lenspec.stop = 0;
//     lenspec.step = 0;
//     lenspec.repeat = 0;
// 
//     // set stream properties
//     s->set(FRAME_Len, 64);
//     // s->set(FRAME_Len, 512);
//     // s->set(FRAME_Len, 1514);
//     s->set(FRAME_Type, ETH_V2);
//     s->set(Network_Hdr, IPv4);
//     // s->set(Transport_Hdr, UDP);
//     s->set(Transport_Hdr, TCP);
// 
//     // s->set(MPLS_01_Stack, 1);
//     // s->set(MPLS_02_Label, 1);
//     // s->set(VLAN_01_Tpi, 1);
//     // s->set(VLAN_01_Vid, 1);
//     
//     // s->set(MAC_Ether_Type, 0x8847);
// 
//     s->set(STREAM_Pkts_Per_Burst, 10); // 1M
//     // s->set(STREAM_Pkts_Per_Burst, 1000000); // 1M
//     // s->set(STREAM_Pkts_Per_Burst, 16'777'216); // 16M
//     // s->set(STREAM_Pkts_Per_Burst, 709'208); // ~700K 
//     s->set(STREAM_Crc_Enable, 1);
// 
//     // add stream to proxy queue
//     p->add_stream(s);
// 
//     // cealog << *s;
// 
//     // start stream processing
//     p->start();
// 
//     return 0;
// }
