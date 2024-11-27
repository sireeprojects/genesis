#include <set>
#include <random>

std::set<int> generateRandomSet(int numValues, int maxValue) {
  std::set<int> values;

  std::random_device rd;
  std::mt19937 gen(rd());

  for (int i = 0; i < numValues; ++i) {
    int value = gen() % maxValue;
    if (values.find(value) == values.end()) { // Check if value is not already in the set
      values.insert(value);
    }
  }

  return values;
}
