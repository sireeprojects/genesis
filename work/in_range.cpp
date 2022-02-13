#include <iostream>
#include <cstdint>
using namespace std;

bool in_range(uint32_t low, uint32_t high, uint32_t x) {        
    return (low <= x && x <= high);         
} 

int main() {
    cout << in_range(10,100,1) << endl;
    return 0;
}
