#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <iomanip>

class DualStream {
private:
    std::ofstream file;
    std::ostream& console;

    // Helper to get current time as a string
    std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time);

        std::stringstream ss;
        ss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";
        return ss.str();
    }

public:
    DualStream(const std::string& filename) 
        : file(filename, std::ios::out | std::ios::app), console(std::cout) {}

    // Method to start a new log entry with a timestamp
    DualStream& startLine() {
        std::string ts = getTimestamp();
        console << ts;
        if (file.is_open()) file << ts;
        return *this;
    }

    template <typename T>
    DualStream& operator<<(const T& data) {
        console << data;
        if (file.is_open()) file << data;
        return *this;
    }

    DualStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        manip(console);
        if (file.is_open()) manip(file);
        return *this;
    }
};

int main() {
    DualStream logger("log.txt");

    // Using .startLine() prepends the time automatically
    logger.startLine() << "Application initialized." << std::endl;
    logger.startLine() << "User ID: " << 42 << " logged in." << std::endl;

    return 0;
}
