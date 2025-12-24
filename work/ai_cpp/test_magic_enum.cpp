#include <iostream>
#include <string_view>

namespace MagicLite {

    template <typename E, E V>
    constexpr std::string_view get_value_name_raw() {
#if defined(__clang__) || defined(__GNUC__)
        std::string_view name = __PRETTY_FUNCTION__;
        size_t start = name.find("V = ");
        if (start == std::string_view::npos) return "";
        start += 4; 

        size_t end = name.find_first_of(" ;]", start);
        std::string_view final_name = name.substr(start, end - start);

        // --- VALIDATION STEP ---
        // If the compiler couldn't find a name, it often returns the number 
        // wrapped in parentheses or just the raw digit. 
        if (final_name.empty() || final_name[0] == '(' || (final_name[0] >= '0' && final_name[0] <= '9')) {
            return ""; 
        }

        size_t last_colon = final_name.find_last_of(':');
        if (last_colon != std::string_view::npos) {
            final_name.remove_prefix(last_colon + 1);
        }
        return final_name;
#elif defined(_MSC_VER)
        // MSVC logic follows a similar pattern
        std::string_view name = __FUNCSIG__;
        // ... (standard MSVC parsing) ...
        return name; 
#endif
    }

    template <typename E, int Min, int Max>
    constexpr std::string_view find_name(int value) {
        if (value == Min) {
            return get_value_name_raw<E, static_cast<E>(Min)>();
        }
        
        if constexpr (Min < Max) {
            return find_name<E, Min + 1, Max>(value);
        } else {
            return ""; // Final fallback if outside Min/Max range
        }
    }

    template <typename E>
    constexpr std::string_view enum_name(E value) {
        return find_name<E, 0, 99>(static_cast<int>(value));
    }
}

// --- Test Section ---

enum class Task {
    Open = 0,
    Active = 2, // Note the gap! 1 is missing.
    Closed = 55
};

enum class EthFields {
    Eth_Open = 1,
    Eth_Active = 2, // Note the gap! 1 is missing.
    Eth_Closed = 23
};

int main() {
    // 1. Valid value
    std::cout << "Value 0: '" << MagicLite::enum_name(Task::Open) << "'" << std::endl;

    // 2. The "Gap" (Value 1 exists in the range 0-50, but not in the enum)
    std::cout << "Value 1 (Gap): '" << MagicLite::enum_name(static_cast<Task>(1)) << "'" << std::endl;

    // 3. Out of Min/Max range (55)
    std::cout << "Value 55 (Limit): '" << MagicLite::enum_name(static_cast<Task>(55)) << "'" << std::endl;

    std::cout << MagicLite::enum_name(static_cast<EthFields>(1)) << std::endl;
    std::cout << MagicLite::enum_name(static_cast<EthFields>(2)) << std::endl;
    std::cout << MagicLite::enum_name(static_cast<EthFields>(23)) << std::endl;

    return 0;
}
