#include <iostream>
#include <random>

using namespace std;

unsigned char *buf;

int main() {
    buf = new unsigned char[100000];

    random_device rd; // obtain a random number from hardware
    mt19937 gen(rd()); // seed the generator
    uniform_int_distribution<> distr(0, 255); // define the range

    for(int i=0; i<100000; i++) {
        buf[i] =  static_cast<char>(distr(gen));
    }
    
    delete(buf);
    return 0;
}
