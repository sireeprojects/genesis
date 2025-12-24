#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <chrono>

// --- THE SLOW WAY (std::string) ---
// This creates a NEW string object for every message (allocates heap memory)
std::string getMessageString(const std::string& line) {
    size_t pos = line.find(':');
    if (pos == std::string::npos) return "";
    return line.substr(pos + 1); 
}

// --- THE FAST WAY (std::string_view) ---
// This just moves a pointer and adjusts a size (Zero allocations)
std::string_view getMessageView(std::string_view line) {
    size_t pos = line.find(':');
    if (pos == std::string_view::npos) return "";
    return line.substr(pos + 1);
}

int main() {
    // A dummy log line
    std::string logLine = "2025-12-21 18:30:05:USER_LOGGED_IN";
    const int iterations = 1000000; // 1 million times

    // Benchmark std::string
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i < iterations; ++i) {
        std::string s = getMessageString(logLine);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diffString = end - start;

    // Benchmark std::string_view
    start = std::chrono::high_resolution_clock::now();
    for(int i=0; i < iterations; ++i) {
        std::string_view sv = getMessageView(logLine);
    }
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diffView = end - start;

    std::cout << "std::string version:      " << diffString.count() << "s\n";
    std::cout << "std::string_view version: " << diffView.count() << "s\n";
    std::cout << "Speedup: " << diffString.count() / diffView.count() << "x faster\n";

    return 0;
}
