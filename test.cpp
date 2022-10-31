#include "cea.h"
using namespace cea;

int main() {
    cea_proxy *p = new cea_proxy(); // proxy instance
    cea_stream *s = new cea_stream(); // stream

    // stream configuration object
    cea_field_generation_spec spec;

    spec = {};
    spec.type = Fixed_Pattern;
    spec.value = 0;
    spec.start = 0;
    spec.stop = 10;
    spec.step = 0;
    spec.repeat = 0;
    spec.pattern = "aa bb cc dd ee ff";

    s->set(FRAME_Len, 100);
    s->set(PAYLOAD_Type, spec);

    s->set(FRAME_Type, ETH_V2);
    s->set(Network_Hdr, IPv4);
    s->set(Transport_Hdr, TCP);
    s->set(STREAM_Pkts_Per_Burst, 10);
    s->set(STREAM_Crc_Enable, 1);

    p->add_stream(s); // add stream to proxy queue
    p->start(); // start stream processing
    return 0;
}
