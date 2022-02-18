#include "cea.h"
using namespace cea;

int main() {
    // proxy instance
    cea_proxy *p = new cea_proxy();

    // stream
    cea_stream *s = new cea_stream();

    // set stream properties
    s->set(FRAME_Len, 100);
    s->set(FRAME_Type, ETH_V2);
    s->set(Network_Hdr, IPv4);
    s->set(Transport_Hdr, UDP);
    // s->set(Transport_Hdr, TCP);

    s->set(MPLS_01_Stack, 1);
    // s->set(MPLS_02_Label, 1);
    // s->set(VLAN_01_Tpi, 1);
    s->set(VLAN_01_Vid, 1);
    
    s->set(MAC_Ether_Type, 0x8847);

    s->set(STREAM_Pkts_Per_Burst, 100);
    s->set(STREAM_Crc_Enable, 1);

    // add stream to proxy queue
    p->add_stream(s);

    // cealog << *s;

    // start stream processing
    p->start();

    return 0;
}
