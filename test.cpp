#include "cea.h"
using namespace cea;

int main() {
    // proxy instance
    cea_proxy *p = new cea_proxy("test_pxy");

    // stream
    cea_stream *s = new cea_stream("test_stm");

    // set stream properties
    s->set(PKT_Type, ETH_V2);
    s->set(MAC_Dest_Addr, 0x112233445566UL);
    s->set(MAC_Src_Addr, 0xaabbccddeeffUL);
    s->set(Network_Hdr, IPv4);
    s->set(Transport_Hdr, UDP);

    cea_mpls_hdr m;
    s->add(MPLS_Hdr, 0, &m);

    cea_vlan_tag v;
    s->add(VLAN_Tag, 0, &v);

    // add stream to proxy queue
    p->add_stream(s);

    // start stream processing
    p->start();

    return 0;
}
