#include <iostream>
#include <cstdint>
using namespace std;


uint32_t gv;
uint32_t minv;
uint32_t maxv;
uint32_t step;
bool repeat;


void check() {
    if ((maxv-minv)%step != 0) {
        cout << "Warning: step value is not perfectly divisible by (max-min)" <<endl;
    }
}

void init() {
    gv = minv;
}

bool increment() {
    cout << gv << endl;
    if ((gv+step) > maxv) {
        if (repeat) {
            gv = minv;
        } else {
            cout << "Done" << endl;
            return false;;
        }
    }
    gv += step;
    return true;
}

int main() {

    minv = 0;
    maxv = 10;
    step = 2;
    repeat = true;

    check();
    init();

    while(increment());

    return 0;
}
