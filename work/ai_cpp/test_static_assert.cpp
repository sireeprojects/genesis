#include <iostream>
#include <type_traits> // Required for type checking
#include <cstdint>

// 1. System Requirement Check
// Ensures the program is only compiled on 64-bit architecture
static_assert(sizeof(void*) == 8, "ERROR: This system must be compiled for 64-bit only!");

// 2. Data Structure Check
struct __attribute__((packed)) Transaction {
// struct Transaction {
    uint64_t id;      // 8 bytes
    double amount;    // 8 bytes
    uint32_t userId;  // 4 bytes
};

// Ensure the struct size is exactly 20 bytes (to detect hidden padding)
// Note: On many compilers, this will FAIL because of alignment, showing static_assert in action!
static_assert(sizeof(Transaction) == 20, "ERROR: Transaction struct has unexpected padding bytes!");

// 3. Template Constraint Check
template <typename T>
void processPayment(T amount) {
    // Rule: Never use floating point (float) for money due to rounding errors
    // Use double or a custom Decimal class instead.
    static_assert(!std::is_same_v<T, float>, "ERROR: Do not use 'float' for currency. Use 'double' for precision.");
    
    std::cout << "Processing payment: " << amount << std::endl;
}

int main() {
    std::cout << "Starting Secure Financial System..." << std::endl;

    double safeAmount = 99.99;
    processPayment(safeAmount);

    // UNCOMMENT the line below to see the compiler error:
    // float riskyAmount = 10.50f;
    // processPayment(riskyAmount); 

    return 0;
}
