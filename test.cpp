#include "cea.h"
using namespace cea;

int main() {
    cea_testbench *tb = new cea_testbench;

    cea_port *port = new cea_port("testPort");
    tb->add_port(port);

    cea_stream *stream = new cea_stream("testStream");
    port->add_stream(stream);

    cea_header *mac =  new cea_header(MAC);
    cea_header *ipv4 = new cea_header(IPv4);
    cea_header *tcp =  new cea_header(TCP);

    // assign a modifier to mac desination address
    cea_gen_spec dest_spec;
    dest_spec.gen_type = Random;
    mac->set(MAC_Dest_Addr, dest_spec);
    // mac->set(MAC_Dest_Addr, "ff:ff:ff:ee:ee:ee");
    mac->set(MAC_Src_Addr, "11:22:33:44:55:aa");
    ipv4->set(IPv4_Dest_Addr, "127.0.0.1");

    stream->set(FRAME_Len, 128);
    // stream->set(STREAM_Start_Delay, 3);

    cea_gen_spec pl_spec;
    pl_spec.gen_type = Increment_Byte;
    // pl_spec.gen_type = Decrement_Byte;
    //
    // pl_spec.gen_type = Increment_Word;
    // pl_spec.gen_type = Decrement_Word;
    //
    // pl_spec.gen_type = Random;
    // pl_spec.gen_type = Fixed_Pattern;
    // pl_spec.pattern = "010203040506070809101112131415";
    // pl_spec.repeat = true;
    stream->set(PAYLOAD_Pattern, pl_spec);

    // add headers in desired seqeunce 
    stream->add_header(mac);
    stream->add_header(ipv4);
    // stream->add_header(tcp);

    tb->start();

    return 0;
}
