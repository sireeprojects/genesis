#include "cea.h"
using namespace cea;

int main() {
    cea_stream s;
    s.set(MAC_Dest_Addr, 0xaabbccddeeffUL);

    cealog << s;
    return 0;
}
