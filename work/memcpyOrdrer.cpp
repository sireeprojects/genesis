#include <iostream>
#include <cstring>
#include <cstdint>
using namespace std;

int main() {

    uint16_t x = 0x0100;

    cout << hex << x << endl;

    cout << hex << (x & 0x0100) << endl;

    return 0;
}
