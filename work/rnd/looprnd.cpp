#include <iostream>
#include <cctype>
#include <random>

using namespace std;

int main(void) {
    random_device rd;
    mt19937_64 engine(rd());
    uniform_int_distribution<unsigned long long> distribute(0, numeric_limits<unsigned long long>::max());

    // for(int repetition=0; repetition<100000000; repetition++) {
    //     distribute(engine);
    // }

    for(int repetition=0; repetition<10; repetition++) {
    cout << distribute(engine) << endl;
    }

  return 0;
}
