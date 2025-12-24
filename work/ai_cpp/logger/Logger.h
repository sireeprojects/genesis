#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <sstream>
#include <iomanip>

// ANSI Color Codes
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"

enum LogLevel { INFO, SUCCESS, WARNING, ERROR };

class DualStream {
private:
    std::ofstream file;
    std::ostream& console;
    std::mutex mtx;
    bool atStartOfLine = true;
    bool isTerminal;

    void injectTimestamp();

public:
    DualStream(const std::string& filename);
    ~DualStream();

    DualStream& level(LogLevel l);

    template <typename T>
    DualStream& operator<<(const T& data) {
        std::lock_guard<std::mutex> lock(mtx);
        injectTimestamp();
        console << data;
        if (file.is_open()) file << data;
        return *this;
    }

    DualStream& operator<<(std::ostream& (*manip)(std::ostream&));
};

// Singleton Access Function
DualStream& get_logger();

// Macro to maintain the 'LOG << ...' syntax
#define LOG get_logger()

#endif
