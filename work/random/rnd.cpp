#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <random>

using namespace std;

int main() {

    random_device rd;
    // mt19937 gen(rd());
    // mt19937 gen(12345);
    mt19937 gen;
    gen.seed(12345); // set the see outside constructor
    uniform_int_distribution<> distr(1,10);


    // for(int i=0; i<5; i++) {
    //     cout << gen() << endl;
    // }

    for(int i=0; i<10; i++) {
        cout << distr(gen) << endl;
    }

    return 0;
}
