#include "cea.h"
using namespace cea;

int main() {
    cea_testbench *tb = new cea_testbench;

    cea_port *port = new cea_port("testPort");
    tb->add_port(port);

    cea_stream *stream = new cea_stream("testStream");
    stream->set(PCAP_Record_Tx_Enable, true);

    port->add_stream(stream);

    cea_header *mac =  new cea_header(MAC);
    cea_header *ipv4 = new cea_header(IPv4);
    cea_header *tcp =  new cea_header(TCP);
    
    cea_field_genspec etype_spec;
    //    // Fixed
    //    etype_spec.gen_type = Fixed_Value;
    //    etype_spec.nmr.value = 0xaabb;
    //    // Value list
    //    etype_spec.gen_type = Value_List;
    //    etype_spec.nmr.values = {0x10,0x20,0x30,0x40,0x50};
    //    mac->set(MAC_Ether_Type, etype_spec);

    // Increment
    etype_spec.gen_type = Increment;
    etype_spec.nmr.start = 0;
    etype_spec.nmr.count = 3;
    etype_spec.nmr.step = 1;
    etype_spec.nmr.repeat = true;
    mac->set(MAC_Ether_Type, etype_spec);

    // assign a modifier to mac desination address
    // cea_field_genspec dest_spec;
    // dest_spec.gen_type = Random;
    // mac->set(MAC_Dest_Addr, dest_spec);
    // mac->set(MAC_Dest_Addr, "ff:ff:ff:ee:ee:ee");
    // mac->set(MAC_Src_Addr, "11:22:33:44:55:66");
    // ipv4->set(IPv4_Dest_Addr, "127.255.255.255");
    // ipv4->set(IPv4_Src_Addr, "127.0.0.1");
    // ipv4->set(IPv4_Protocol, 2);

    stream->set(FRAME_Len, 128);

    cea_field_genspec pl_spec;
    pl_spec.gen_type = Increment_Byte;
    // pl_spec.gen_type = Decrement_Byte;
    // pl_spec.gen_type = Increment_Word;
    // pl_spec.gen_type = Decrement_Word;
    // pl_spec.gen_type = Random;
    // pl_spec.gen_type = Fixed_Pattern;
    // pl_spec.pattern = "010203040506070809101112131415"; // TODO
    // pl_spec.repeat = true;
    stream->set(PAYLOAD_Pattern, pl_spec);

    // add headers in desired seqeunce 
    stream->add_header(mac);
    stream->add_header(ipv4);
    // stream->add_header(tcp);

    tb->start();
    return 0;
}
