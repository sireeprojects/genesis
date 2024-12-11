#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <climits>
#include <iomanip>

using namespace std;

int main() {

    // user inputs
    uint32_t nof_frames = 20;
    uint32_t start = 20;
    uint32_t step = 3;
    uint32_t count = 8;
    // bool repeat = true;
    bool repeat = false;

    // runtime
    uint32_t rt_count = 0;
    uint32_t rt_value = 0;
    
    // prep_for_mutation
    rt_value = start;
    rt_count = 0;

    // mutation
    for (uint32_t nf=0; nf<nof_frames; nf++) {
        cout << "nf:" << setw(2) << nf << "  " <<  setw(16) << rt_value << endl;
        if (rt_count == count) {
            if (repeat) {
                rt_count = 0;
                rt_value = start;
            }
        } else {
            if ( (rt_value-step) > start) { // CHECK this condition thoroughly
                rt_value = start;
            } else {
                rt_value -= step;
                rt_count++;
            }
        }
    }

    return 0;
}
