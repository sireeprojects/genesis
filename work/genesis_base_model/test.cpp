#include "cea.h"
using namespace cea;

int main() {
    cea_proxy *p = new cea_proxy();
    cea_stream *s = new cea_stream();

    p->add_stream(s);
    p->start();
    return 0;
}
