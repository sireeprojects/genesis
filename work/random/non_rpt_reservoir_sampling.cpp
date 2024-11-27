#include <vector>
#include <random>

std::vector<int> generateRandomReservoir(int numValues, int maxValue) {
  std::vector<int> reservoir(numValues);

  std::random_device rd;
  std::mt19937 gen(rd());

  for (int i = 0; i < maxValue; ++i) {
    int value = gen() % maxValue;
    int j = gen() % numValues; // Random index to replace
    if (j < numValues) { // Avoid out-of-bounds access
      reservoir[j] = value;
    }
  }

  return reservoir;
}
