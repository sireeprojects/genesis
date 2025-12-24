#include <iostream>
#include <fstream>
#include <string>

class DualStream {
private:
    std::ofstream file;
    std::ostream& console;

public:
    // Constructor: opens the file and stores a reference to the console
    DualStream(const std::string& filename) 
        : file(filename, std::ios::out | std::ios::app), console(std::cout) {
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
        }
    }

    // Overload the << operator for any type T
    // This allows it to work with strings, ints, doubles, etc.
    template <typename T>
    DualStream& operator<<(const T& data) {
        console << data; // Print to display
        if (file.is_open()) {
            file << data; // Save to file
        }
        return *this; // Return itself to allow chaining (e.g., stream << a << b)
    }

    // Special handler for manipulators like std::endl
    DualStream& operator<<(std::ostream& (*manip)(std::ostream&)) {
        manip(console);
        if (file.is_open()) {
            manip(file);
        }
        return *this;
    }
};

int main() {
    DualStream logger("log.txt");

    // Usage feels just like std::cout
    logger << "System Start: " << 2025 << "\n";
    logger << "Writing to both console and file..." << std::endl << "some more";

    return 0;
}
