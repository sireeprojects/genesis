#include <iostream>
#include <variant>
#include <string>
#include <cstdint> // Required for int64_t

using namespace std;

int main() {
    // Define a variant that can hold a 64-bit integer or a string
    // int64_t ensures exactly 64 bits of precision
    std::variant<int64_t, std::string> myData;
    uint64_t ival;
    string sval;

    // --- Scenario 1: Storing a 64-bit Integer ---
    myData = 9223372036854775807LL; // Maximum value for a signed 64-bit int

    std::cout << "Checking first assignment..." << std::endl;

    if (std::holds_alternative<int64_t>(myData)) {
        std::cout << "Currently holding an int64: " << std::get<int64_t>(myData) << std::endl;
        ival = std::get<int64_t>(myData);
        std::cout << "Currently holding an int64: " << ival << std::endl;
    } else if (std::holds_alternative<std::string>(myData)) {
        std::cout << "Currently holding a string: " << std::get<std::string>(myData) << std::endl;
        sval = std::get<std::string>(myData);
        std::cout << "Currently holding a string: " << sval << std::endl;
    }

    std::cout << "----------------------------" << std::endl;

    // --- Scenario 2: Storing a String ---
    myData = "Standard Template Library";

    std::cout << "Checking second assignment..." << std::endl;

    if (std::holds_alternative<int64_t>(myData)) {
        std::cout << "Currently holding an int64: " << std::get<int64_t>(myData) << std::endl;
    } else if (std::holds_alternative<std::string>(myData)) {
        // Safe access because we verified it's a string
        std::cout << "Currently holding a string: " << std::get<std::string>(myData) << std::endl;
        sval = std::get<std::string>(myData);
        std::cout << "Currently holding a string: " << sval << std::endl;
    }

    return 0;
}
