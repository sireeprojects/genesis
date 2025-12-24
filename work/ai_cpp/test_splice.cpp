#include <iostream>
#include <vector>
#include <cstdint>
#include <algorithm>
#include <cstring>

struct BitEntry {
    uint32_t size_in_bits;
    uint32_t offset;
    uint64_t value;
};

/**
 * Splices bits into a heap-allocated char array.
 * @param out_size: Pointer to an integer to receive the final buffer length.
 * @return: Pointer to the allocated char array. CALLER MUST CALL delete[].
 */
char* splice_bits_to_ptr(const std::vector<BitEntry>& fields, size_t* out_size) {
    if (fields.empty()) {
        if (out_size) *out_size = 0;
        return nullptr;
    }

    // 1. Calculate required buffer size
    uint32_t max_bit = 0;
    for (const auto& f : fields) {
        max_bit = std::max(max_bit, f.offset + f.size_in_bits);
    }
    
    size_t total_bytes = (max_bit + 7) / 8;
    if (out_size) *out_size = total_bytes;

    // 2. Allocate on the heap
    // We use value-initialization {0} to zero out the memory
    char* buffer = new char[total_bytes]{0};

    // 3. Splice using word-level shifts
    for (const auto& f : fields) {
        if (f.size_in_bits == 0) continue;

        uint32_t bits_processed = 0;
        while (bits_processed < f.size_in_bits) {
            uint32_t current_offset = f.offset + bits_processed;
            uint32_t byte_idx = current_offset / 8;
            uint32_t bit_in_byte = current_offset % 8;

            // Process in chunks of up to 56 bits (to ensure we fit in 64-bit word after shifting)
            uint32_t bits_to_write = std::min(f.size_in_bits - bits_processed, 56u - bit_in_byte);
            
            uint64_t mask = (bits_to_write >= 64) ? ~0ULL : (1ULL << bits_to_write) - 1;
            uint64_t val_to_write = (f.value >> bits_processed) & mask;

            // Safe load/store using memcpy to handle potential alignment issues and raw char*
            uint64_t target_word = 0;
            size_t bytes_remaining = total_bytes - byte_idx;
            size_t copy_size = std::min(bytes_remaining, size_t(8));
            
            std::memcpy(&target_word, &buffer[byte_idx], copy_size);

            // Punch the hole and insert value
            target_word &= ~(mask << bit_in_byte);
            target_word |= (val_to_write << bit_in_byte);

            std::memcpy(&buffer[byte_idx], &target_word, copy_size);

            bits_processed += bits_to_write;
            if (bits_to_write == (f.size_in_bits - bits_processed)) break;
        }
    }

    return buffer;
}

int main() {
    std::vector<BitEntry> data = {
        {4,  0,  0xA},        // 4-bit Version
        {4,  4,  0x5},        // 4-bit Header Length
        {8,  8,  0x40},       // 8-bit Type of Service
        {16, 16, 0x0100},     // 16-bit Total Length (spanning 2 bytes)
        {1,  32, 1},          // 1-bit Flag A
        {1,  33, 0},          // 1-bit Flag B
        {1,  34, 1},          // 1-bit Flag C
        {13, 35, 0x1FFF},     // 13-bit Fragment Offset (crosses byte boundary)
        {32, 48, 0xDEADBEEF}, // 32-bit Identifier
        {8,  80, 0xFF}        // 8-bit TTL (Time to Live)
    };

    size_t buffer_size = 0;
    char* raw_ptr = splice_bits_to_ptr(data, &buffer_size);

    if (raw_ptr) {
        printf("Buffer Size: %zu bytes\n", buffer_size);
        printf("Hex Content: ");
        for (size_t i = 0; i < buffer_size; ++i) {
            printf("%02X ", (uint8_t)raw_ptr[i]);
        }
        printf("\n");

        // IMPORTANT: Clean up heap memory
        delete[] raw_ptr;
    }

    return 0;
}
