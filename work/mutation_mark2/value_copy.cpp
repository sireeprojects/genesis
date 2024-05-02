#include <iostream>
#include <cstring>
#include <string>
#include <cstdint>
#include <iomanip>
#include "print_cdata.h"

using namespace std;

int main() {
    // uint64_t value = 0x8000;
    uint64_t value = 100;
    unsigned char pf[100];
    memset(pf, 0, 100);
    memcpy(pf, (char*)&value, 1);

    print_cdata(pf, 100);
    return 0;
}
