#include <iostream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <iomanip>
#include <memory>

struct BitEntry {
    uint32_t size_in_bits;
    uint32_t offset;
    uint64_t value;
};

class BitSplicer {
public:
    // Serializes a vector of entries into a raw buffer
    static std::unique_ptr<char[]> serialize(const std::vector<BitEntry>& fields, size_t& out_size) {
        uint32_t max_bit = 0;
        for (const auto& f : fields) {
            validate(f);
            max_bit = std::max(max_bit, f.offset + f.size_in_bits);
        }

        out_size = (max_bit + 7) / 8;
        auto buffer = std::make_unique<char[]>(out_size);
        std::memset(buffer.get(), 0, out_size);

        for (const auto& f : fields) {
            pack(buffer.get(), out_size, f);
        }
        return buffer;
    }

    // Deserializes/Extracts a specific value from a raw buffer
    static uint64_t deserialize(const char* buffer, size_t buffer_size, uint32_t offset, uint32_t size_in_bits) {
        uint32_t byte_idx = offset / 8;
        uint32_t bit_in_byte = offset % 8;

        if (byte_idx >= buffer_size) return 0;

        uint64_t window = 0;
        size_t copy_len = std::min(buffer_size - byte_idx, (size_t)8);
        std::memcpy(&window, &buffer[byte_idx], copy_len);

        uint64_t result = window >> bit_in_byte;
        uint64_t mask = (size_in_bits >= 64) ? ~0ULL : (1ULL << size_in_bits) - 1;
        
        return result & mask;
    }

private:
    static void validate(const BitEntry& entry) {
        if (entry.size_in_bits < 64) {
            uint64_t max_val = (1ULL << entry.size_in_bits) - 1;
            if (entry.value > max_val) {
                throw std::out_of_range("Value exceeds bit-field capacity.");
            }
        }
    }

    static void pack(char* buffer, size_t buffer_size, const BitEntry& f) {
        uint32_t bits_written = 0;
        while (bits_written < f.size_in_bits) {
            uint32_t current_pos = f.offset + bits_written;
            uint32_t byte_idx = current_pos / 8;
            uint32_t bit_in_byte = current_pos % 8;

            // Stay within 56 bits to prevent overflow during the shift
            uint32_t chunk_size = std::min(f.size_in_bits - bits_written, 56u - bit_in_byte);
            uint64_t mask = (chunk_size >= 64) ? ~0ULL : (1ULL << chunk_size) - 1;
            uint64_t val_chunk = (f.value >> bits_written) & mask;

            uint64_t window = 0;
            size_t copy_len = std::min(buffer_size - byte_idx, (size_t)8);
            std::memcpy(&window, &buffer[byte_idx], copy_len);

            window &= ~(mask << bit_in_byte);
            window |= (val_chunk << bit_in_byte);

            std::memcpy(&buffer[byte_idx], &window, copy_len);
            bits_written += chunk_size;
        }
    }
};

// --- Execution Example ---

int main() {
    try {
        std::vector<BitEntry> packet_config = {
            {4,  0,  0x9}, {4, 4, 0x2}, {8, 8, 0xAA}, {16, 16, 0xDEAD},
            {1,  32, 1},   {1, 33, 1},  {1, 34, 0},   {13, 35, 0x0ABC},
            {32, 48, 0xFEEDFACE},       {8, 80, 0x12}
        };

        size_t buf_size = 0;
        auto buffer = BitSplicer::serialize(packet_config, buf_size);

        std::cout << "Spliced " << buf_size << " bytes successfully.\n\n";

        // Verification Table
        std::cout << std::left << std::setw(8) << "Index" 
                  << std::setw(15) << "Expected" 
                  << std::setw(15) << "Extracted" 
                  << "Status\n" << std::string(50, '-') << "\n";

        for (size_t i = 0; i < packet_config.size(); ++i) {
            uint64_t val = BitSplicer::deserialize(buffer.get(), buf_size, 
                                                   packet_config[i].offset, 
                                                   packet_config[i].size_in_bits);
            
            std::cout << std::setw(8) << i 
                      << "0x" << std::hex << std::setw(13) << packet_config[i].value 
                      << "0x" << std::hex << std::setw(13) << val 
                      << (val == packet_config[i].value ? "MATCH" : "FAIL") << "\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}


