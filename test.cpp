#include "cea.h"
using namespace cea;

// Expert Mode of stream specification

int main() {
    cea_proxy *proxy = new cea_proxy(); // proxy instance
    cea_stream *stream = new cea_stream(); // stream

    cea_header *mac = stream->create_header(MAC);
    cea_header *ipv4 = stream->create_header(IPv4);
    cea_header *tcp = stream->create_header(TCP);
    cea_header *meta = stream->create_header(META);

    // assign a modifier to mac desination address
    cea_gen_spec dest_spec;
    dest_spec.gen_type = Random;
    mac->set(MAC_Dest_Addr, dest_spec);

    stream->add_header(mac);
    stream->add_header(ipv4);
    stream->add_header(meta);
    stream->add_header(tcp);

    stream->test();

    return 0;
}
