#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <cstdint>

class MacHandler {
private:
    // Mask to keep the value strictly within 48 bits (6 bytes)
    static const uint64_t MAC_MASK = 0xFFFFFFFFFFFFULL;

public:
    static uint64_t stringToLong(const std::string& mac) {
        std::string hexOnly = "";
        for (char c : mac) {
            if (isxdigit(c)) hexOnly += c;
        }
        return std::stoull(hexOnly, nullptr, 16) & MAC_MASK;
    }

    static std::string longToString(uint64_t val) {
        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::uppercase;
        for (int i = 5; i >= 0; --i) {
            ss << std::setw(2) << ((val >> (i * 8)) & 0xFF);
            if (i > 0) ss << ":";
        }
        return ss.str();
    }
};

int main() {
    std::string startMac = "00:1A:2B:3C:4D:5E";
    uint64_t currentMac = MacHandler::stringToLong(startMac);

    std::cout << "--- Incrementing 100 Times ---" << std::endl;
    for (int i = 1; i <= 100; ++i) {
        currentMac = (currentMac + 1) & 0xFFFFFFFFFFFFULL; // Increment and mask
        std::cout << i << ": " << MacHandler::longToString(currentMac) << "\n";
    }

    std::cout << "\n--- Decrementing 100 Times ---" << std::endl;
    for (int i = 1; i <= 100; ++i) {
        currentMac = (currentMac - 1) & 0xFFFFFFFFFFFFULL; // Decrement and mask
        std::cout << i << ": " << MacHandler::longToString(currentMac) << "\n";
    }

    return 0;
}
