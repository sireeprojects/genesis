#include <iostream>
#include <cctype>
#include <random>
#include <cmath>

using namespace std;

using u32    = uint_least32_t; 
using u64    = uint_least64_t; 
using engine = std::mt19937;

int main(void) {
  std::random_device os_seed;
  const u32 seed = os_seed();
  
  engine generator(seed);
  // std::uniform_int_distribution<u32> distribute(1, (pow(2,32)-1));
  std::uniform_int_distribution<u64> distribute(1, (pow(2,64)-1));

  // for(int repetition=0; repetition<100000000; repetition++)
    // distribute(generator);

  for(int repetition=0; repetition<10; repetition++)
    cout << distribute(generator) << endl;

  return 0;
}
