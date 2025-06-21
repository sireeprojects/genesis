#include <iostream>
#include <random>
#include <cstdint>
#include <string>
 
using namespace std;

struct cea_field_random {
    uint64_t seed;
    mt19937 engine;
    uniform_int_distribution<> udist;
    discrete_distribution<> wdist;
};

int main() {
    random_device rd;
    cea_field_random rnd;
    
    rnd.seed = rd();
    // rnd.seed = 12345;

    cout << string(30, '-') << endl;
    cout << "Seed: " << rnd.seed << endl;

    rnd.engine.seed(rnd.seed);

    // WEIGHTED Distribution
    vector<double>weights = {1.1, 3.3, 0.5, 2.2};
    discrete_distribution<>::param_type wd(weights.begin(), weights.end());
    rnd.wdist.param(wd);
    cout << string(30, '-') << endl;
    uint32_t idx;
    for (uint32_t i=0; i<10; i++) {
        idx = rnd.wdist(rnd.engine);
        cout << weights[idx] << endl;
    }

    // UNIFORM Distribution
    uniform_int_distribution<int>::param_type d2(2, 10);
    rnd.udist.param(d2);
    cout << rnd.udist << endl;

    cout << string(30, '-') << endl;
    for (uint32_t i=0; i<10; i++)
        cout << rnd.udist(rnd.engine) << endl;

    return 0;
}
