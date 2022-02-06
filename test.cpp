#include "cea.h"
using namespace cea;

int main() {
    // proxy instance
    // cea_proxy p("test_proxy");
    cea_proxy p;

    // stream
    // cea_stream s("test_stream");
    cea_stream s;

    // set stream properties
    s.set(PKT_Type, ETH_V2);
    s.set(Network_Hdr, IPv4);
    s.set(Transport_Hdr, UDP);
    // s.add(VLAN_Tags);
    // s.add(MPLS_Labels);
    s.set(MAC_Dest_Addr, 0x112233445566UL);
    s.set(MAC_Src_Addr, 0xaabbccddeeffUL);

    // add stream to proxy queue
    p.add_stream(&s);

    // invoke debug function
    p.start();

    return 0;
}
