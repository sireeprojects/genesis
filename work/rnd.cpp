#include <iostream>
#include <random>
#include <cstdint>
#include <string>
 
using namespace std;

struct cea_field_random {
    uint64_t seed;
    mt19937 engine;
    uniform_int_distribution<> distr;
    // discrete_distribution<> distr;
};

int main() {
    random_device rd;
    cea_field_random rnd;
    
    // rnd.seed = rd();
    rnd.seed = 12345;

    cout << string(30, '-') << endl;
    cout << "Seed: " << rnd.seed << endl;

    rnd.engine.seed(rnd.seed);

    uniform_int_distribution<int>::param_type d2(2, 10);
    rnd.distr.param(d2);
    cout << rnd.distr;

    for (uint32_t i=0; i<10; i++)
        cout << rnd.distr(rnd.engine) << endl;

    return 0;
}
