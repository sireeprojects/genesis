#include "cea.h"
using namespace cea;

int main() {
    // proxy instance
    cea_proxy p("testproxy");

    // stream
    cea_stream s;

    // set stream properties
    s.set(PKT_Type, Ethernet_V2);
    s.set(PKT_Network_Hdr, IPv6);
    s.set(PKT_Transport_Hdr, UDP);
    s.add(PKT_VLAN_Tags);
    s.add(PKT_MPLS_Labels);

    // add stream to proxy queue
    p.add_stream(&s);

    // invoke debug function
    p.testfn();

    return 0;
}
