#include "cea.h"
using namespace cea;

int main() {
    cea_proxy *p = new cea_proxy(); // proxy instance
    cea_stream *s = new cea_stream(); // stream

    cea_header *mac = new cea_header(TCP);
    return 0;
}
