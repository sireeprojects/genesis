#include <algorithm>
#include <vector>
#include <random>

std::vector<int> generateRandomSequence(int numValues, int maxValue) {
  std::vector<int> values(numValues);
  std::iota(values.begin(), values.end(), 0); // Initialize with sequential values

  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(values.begin(), values.end(), gen);

  return values;
}
