#include <random>
#include <iostream>

int main() {
    // Seed the random number generator
    std::random_device rd;
    // std::mt19937 gen(rd());
    std::mt19937 gen(1234); // 1234 = seed

    // Define the range of numbers (0-99)
    std::uniform_int_distribution<int> dis(0, 99);

    // Generate 10 repeatable random numbers
    for (int i = 0; i < 10; i++) {
        int random_number = dis(gen);
        std::cout << random_number << " ";
    }
    std::cout << std::endl;

    return 0;
}
