#include <iostream>
#include <cstdint>
#include <vector>
using namespace std;

int main() {
    vector <uint32_t> fseq = {1,2,3,4,5};

    uint32_t cntr = 0;
    for (auto i: fseq) {
        if (cntr!=0) {
        auto prev = fseq.begin() + (cntr-1);
        cout << "i=" << i << "  " << "prev=" << *prev << "  " << "Cntr=" << cntr << endl; 
        }
        cntr++;
    }
    return 0;
}
