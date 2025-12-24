#include <iostream>
#include <utility>   // Required for std::in_range
#include <cstdint>   // For fixed-width types
#include <vector>

/**
 * A helper function to print the result of an in_range check.
 * Note: std::in_range requires the template parameter to be an integer type.
 */
template <typename TargetType, typename ValueType>
void check_and_print(ValueType val, const std::string& type_name) {
    bool fits = std::in_range<TargetType>(val);
    
    std::cout << "Value: " << std::showpos << val 
              << " \tFits in " << type_name << "? " 
              << std::noshowpos << (fits ? "[ YES ]" : "[ NO  ]") 
              << std::endl;
}

int main() {
    std::cout << "--- C++20 std::in_range Demonstration ---\n" << std::endl;

    // 1. Testing against uint8_t (Range: 0 to 255)
    std::cout << "Checking limits for uint8_t (0 to 255):" << std::endl;
    check_and_print<uint8_t>(255,   "uint8_t");
    check_and_print<uint8_t>(256,   "uint8_t");
    check_and_print<uint8_t>(-1,    "uint8_t");
    std::cout << "-----------------------------------------" << std::endl;

    // 2. Testing against int16_t (Range: -32,768 to 32,767)
    std::cout << "Checking limits for int16_t:" << std::endl;
    check_and_print<int16_t>(32767,  "int16_t");
    check_and_print<int16_t>(40000,  "int16_t");
    check_and_print<int16_t>(-32768, "int16_t");
    std::cout << "-----------------------------------------" << std::endl;

    // 3. Testing large signed vs unsigned transitions
    // This is where standard (x < limit) comparisons often fail due to sign-extension
    long long huge_val = 5000000000LL; // 5 Billion
    std::cout << "Checking 5 Billion:" << std::endl;
    check_and_print<uint32_t>(huge_val, "uint32_t"); // Too big for 32-bit (max ~4.2B)
    check_and_print<uint64_t>(huge_val, "uint64_t"); // Fits in 64-bit
    
    return 0;
}
