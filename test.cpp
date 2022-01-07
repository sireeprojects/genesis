#include "cea.h"
using namespace cea;

int main() {
    // proxy instance
    cea_proxy p("testproxy");

    // stream
    cea_stream s("teststream");

    // set stream properties
    s.set(PKT_Type, Ethernet_V2);
    // s.set(PKT_Network_Hdr, IPv6);
    // s.set(PKT_Transport_Hdr, UDP);
    // s.add(PKT_VLAN_Tags);
    // s.add(PKT_MPLS_Labels);
    // s.set(MAC_Dest_Addr, 0x112233445566UL);

    // add stream to proxy queue
    p.add_stream(&s);

    // invoke debug function
    p.start();

    return 0;
}
