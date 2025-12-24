#include <iostream>
#include <string>
#include <iomanip>
#include <sstream>
#include <cstdint>

class MacHandler {
private:
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
    // Starting near a "roll-over" point to show multiple digits changing
    std::string startMac = "00:FF:FF:FF:FF:9C"; 
    uint64_t currentMac = MacHandler::stringToLong(startMac);

    std::cout << "Initial MAC: " << startMac << "\n\n";

    // --- Incrementing 100 Times ---
    std::cout << "--- Incrementing 100 Times (Watch the roll-over) ---" << std::endl;
    for (int i = 1; i <= 100; ++i) {
        currentMac = (currentMac + 1) & 0xFFFFFFFFFFFFULL;
        // The transition from ...FF:FF to ...00:00 will happen at iteration 100
        std::cout << std::setw(3) << i << ": " << MacHandler::longToString(currentMac) << "\n";
    }

    std::cout << "\n--- Decrementing 100 Times (Watch it roll back) ---" << std::endl;
    for (int i = 1; i <= 100; ++i) {
        currentMac = (currentMac - 1) & 0xFFFFFFFFFFFFULL;
        std::cout << std::setw(3) << i << ": " << MacHandler::longToString(currentMac) << "\n";
    }

    return 0;
}
