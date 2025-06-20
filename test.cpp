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
    
    cea_field_genspec mac_spec;

    // mac_spec.gen_type = Fixed_Value;
    // mac_spec.str.value = "aa:bb:cc:dd:ee:ff";
    // mac->set(MAC_Dest_Addr, mac_spec);
    
    mac_spec.gen_type = Value_List;
    mac_spec.str.values = {
        "11:22:33:44:55:66",
        "aa:bb:cc:dd:ee:ff",
        "23:34:45:56:67:78"
    };
    mac_spec.str.repeat = true;
    mac->set(MAC_Dest_Addr, mac_spec);


    // Increment/Decrement
    // mac_spec.gen_type = Increment;
    // // mac_spec.gen_type = Decrement;
    // mac_spec.str.start = "11:22:33:44:55:00";
    // mac_spec.str.count = 20;
    // mac_spec.str.step = 1;
    // mac_spec.str.repeat = true;
    // mac->set(MAC_Dest_Addr, mac_spec);
    

    // cea_field_genspec len_spec;
    // len_spec.gen_type = Weighted_Distribution;
    // len_spec.nmr.distr_name = "iMIX";
    // len_spec.nmr.distr = {
    //                      {64, 20},
    //                      {512, 70},
    //                      {1500, 10} };
    // stream->set(FRAME_Len, len_spec);

    stream->set(FRAME_Len, 128);
    stream->set(STREAM_Burst_Size, 20);

    cea_field_genspec payload_spec;
    payload_spec.gen_type = Increment_Byte;
    stream->set(PAYLOAD_Pattern, payload_spec);

    // add headers in desired seqeunce 
    stream->add_header(mac);
    stream->add_header(ipv4);

    tb->start();
    return 0;
}


// int main() {
//     cea_testbench *tb = new cea_testbench;
// 
//     cea_port *port = new cea_port("testPort");
//     tb->add_port(port);
// 
//     cea_stream *stream = new cea_stream("testStream");
//     stream->set(PCAP_Record_Tx_Enable, true);
// 
//     port->add_stream(stream);
// 
//     cea_header *mac =  new cea_header(MAC);
//     cea_header *ipv4 = new cea_header(IPv4);
//     cea_header *tcp =  new cea_header(TCP);
//     
//     cea_field_genspec etype_spec;
// 
//     // Fixed
//     // etype_spec.gen_type = Fixed_Value;
//     // etype_spec.nmr.value = 0xaabb;
//     // mac->set(MAC_Ether_Type, etype_spec);
// 
//     // Value list
//     // etype_spec.gen_type = Value_List;
//     // etype_spec.nmr.values = {0x10,0x20,0x30,0x40,0x50};
//     // etype_spec.nmr.repeat = true;
//     // mac->set(MAC_Ether_Type, etype_spec);
// 
//     // Increment/Decrement
//     etype_spec.gen_type = Increment;
//     // etype_spec.gen_type = Decrement;
//     etype_spec.nmr.start = 0x0000;
//     etype_spec.nmr.count = 20;
//     etype_spec.nmr.step = 1;
//     // etype_spec.nmr.repeat = true;
//     mac->set(MAC_Ether_Type, etype_spec);
// 
//     // assign a modifier to mac desination address
//     // cea_field_genspec dest_spec;
//     // dest_spec.gen_type = Random;
//     // mac->set(MAC_Dest_Addr, dest_spec);
//     // mac->set(MAC_Dest_Addr, "ff:ff:ff:ee:ee:ee");
//     // mac->set(MAC_Src_Addr, "11:22:33:44:55:66");
//     // ipv4->set(IPv4_Dest_Addr, "127.255.255.255");
//     // ipv4->set(IPv4_Src_Addr, "127.0.0.1");
//     // ipv4->set(IPv4_Protocol, 2);
// 
//     stream->set(FRAME_Len, 128);
//     stream->set(STREAM_Burst_Size, 20);
// 
//     cea_field_genspec payload_spec;
//     payload_spec.gen_type = Increment_Byte;
//     // payload_spec.gen_type = Decrement_Byte;
//     // payload_spec.gen_type = Increment_Word;
//     // payload_spec.gen_type = Decrement_Word;
//     // payload_spec.gen_type = Random;
//     // payload_spec.gen_type = Fixed_Pattern;
//     // payload_spec.pattern = "010203040506070809101112131415"; // TODO
//     // payload_spec.repeat = true;
//     stream->set(PAYLOAD_Pattern, payload_spec);
// 
//     // add headers in desired seqeunce 
//     stream->add_header(mac);
//     stream->add_header(ipv4);
//     // stream->add_header(tcp);
// 
//     tb->start();
//     return 0;
// }
