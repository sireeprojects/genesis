#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <iomanip>

using namespace std;

int main() {

    vector<uint32_t> list = {1,2,3,4,5};
    // bool repeat = true;
    bool repeat = false;

    uint32_t nof_frames = 20;
    uint32_t frames_gen = 0;
    uint32_t list_idx = 0;


    // mutation
    while (frames_gen <= nof_frames) {
        cout << list[list_idx] << "  " << endl;
        if (list_idx == list.size()-1) {
            if (repeat) {
                list_idx = 0;
            } else {
                break;
            }
        } else {
            list_idx++;
        }
    }

    return 0;
}
