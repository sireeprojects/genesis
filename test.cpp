#include "cea.h"
using namespace cea;

int main() {
    // proxy instance
    cea_proxy *p = new cea_proxy("test_pxy");

    // stream
    cea_stream *s = new cea_stream("test_stm");

    // set stream properties
    s->set(PKT_Type, ETH_V2);
    // s->set(MAC_Dest_Addr, 0x112233445566UL);
    // s->set(MAC_Src_Addr, 0xaabbccddeeffUL);
    s->set(Network_Hdr, ARP);
    s->set(Transport_Hdr, UDP);

    // s->set(MPLS_01_Ttl, 1);
    // s->set(MPLS_02_Label, 1);
    // s->set(MPLS_03_Label, 1);
    // s->set(VLAN_01_Tpi, 1);
    // s->set(VLAN_02_Vid, 1);

    // add stream to proxy queue
    p->add_stream(s);

    // start stream processing
    p->start();

    return 0;
}
