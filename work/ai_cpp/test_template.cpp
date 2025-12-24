#include <iostream>
#include <chrono>
#include <vector>
#include <cstdint>

// --- Common Data ---
struct Packet {
    uint8_t data[64];
};

// --- Approach 1: Runtime Enum & Switch ---
enum FieldID { ETH_TYPE, IP_VER, IP_IHL, IP_TTL };

void set_field_enum(Packet& p, FieldID id, uint32_t val) {
    // The CPU must evaluate this switch at runtime for every call
    switch (id) {
        case ETH_TYPE: 
            *reinterpret_cast<uint16_t*>(&p.data[12]) = static_cast<uint16_t>(val); 
            break;
        case IP_VER:   
            p.data[14] = (p.data[14] & 0x0F) | ((val & 0x0F) << 4); 
            break;
        case IP_IHL:   
            p.data[14] = (p.data[14] & 0xF0) | (val & 0x0F); 
            break;
        case IP_TTL:   
            p.data[22] = static_cast<uint8_t>(val); 
            break;
    }
}

// --- Approach 2: Compile-time Tags ---
struct TagVer { static constexpr size_t off = 14; static constexpr uint8_t b_off = 4; static constexpr uint8_t b_len = 4; };
struct TagTTL { static constexpr size_t off = 22; static constexpr uint8_t b_off = 0; static constexpr uint8_t b_len = 8; };

template<typename Tag>
inline void set_field_tag(Packet& p, uint32_t val) {
    if constexpr (Tag::b_len == 8) {
        p.data[Tag::off] = static_cast<uint8_t>(val);
    } else {
        constexpr uint8_t mask = (1 << Tag::b_len) - 1;
        p.data[Tag::off] = (p.data[Tag::off] & ~(mask << Tag::b_off)) | ((val & mask) << Tag::b_off);
    }
}

int main() {
    const int iterations = 100'000'000; // 100 Million packets
    Packet pkt;

    // --- Benchmark Enum/Switch ---
    auto s1 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        set_field_enum(pkt, IP_VER, 4);
        set_field_enum(pkt, IP_TTL, 64);
    }
    auto e1 = std::chrono::high_resolution_clock::now();

    // --- Benchmark Tag/Template ---
    auto s2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        set_field_tag<TagVer>(pkt, 4);
        set_field_tag<TagTTL>(pkt, 64);
    }
    auto e2 = std::chrono::high_resolution_clock::now();

    // Results
    std::chrono::duration<double> d1 = e1 - s1;
    std::chrono::duration<double> d2 = e2 - s2;

    std::cout << "Runtime Enum/Switch: " << d1.count() << " seconds" << std::endl;
    std::cout << "Compile-time Tag:    " << d2.count() << " seconds" << std::endl;
    std::cout << "Improvement:         " << (d1.count() / d2.count()) << "x faster" << std::endl;

    return 0;
}
