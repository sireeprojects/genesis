#include "cea.h"
using namespace cea;

int main() {
    cea_proxy *proxy = new cea_proxy(); // proxy instance
    cea_stream *stream = new cea_stream(); // stream

    cea_header *mac = stream->create_header(MAC);
    stream->add_header(mac);

    return 0;
}
