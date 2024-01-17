#include "cea.h"
using namespace cea;

// Expert Mode of stream specification

int main() {
    cea_testbench *tb = new cea_testbench;

    cea_port *port = new cea_port("testPort");
    tb->add_port(port);

    cea_stream *stream = new cea_stream("testStream");
    port->add_stream(stream);

    cea_header *mac =  new cea_header(MAC);
    cea_header *ipv4 = new cea_header(IPv4);
    cea_header *tcp =  new cea_header(TCP);
    cea_header *meta = new cea_header(META);

    // assign a modifier to mac desination address
    cea_gen_spec dest_spec;
    dest_spec.gen_type = Random;
    mac->set(MAC_Dest_Addr, dest_spec);

    stream->set(FRAME_Len, 100);

    // add headers in desired seqeunce 
    stream->add_header(mac);
    stream->add_header(ipv4);
    stream->add_header(meta);
    stream->add_header(tcp);

    tb->start();

    return 0;
}
