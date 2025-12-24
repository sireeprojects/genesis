#include "Logger.h"
#include <chrono>
#include <ctime>
#include <unistd.h>

// The Singleton Implementation
DualStream& get_logger() {
    // Guaranteed to be thread-safe and initialized on first use (C++11+)
    static DualStream instance("library_log.txt");
    return instance;
}

DualStream::DualStream(const std::string& filename) : console(std::cout) {
    file.open(filename, std::ios::out | std::ios::app);
    isTerminal = isatty(fileno(stdout));
}

DualStream::~DualStream() {
    if (file.is_open()) {
        file << "[SHUTDOWN] Logger closed." << std::endl;
        file.close();
    }
}

void DualStream::injectTimestamp() {
    if (atStartOfLine) {
        auto now = std::chrono::system_clock::now();
        std::time_t time = std::chrono::system_clock::to_time_t(now);
        
        // Safety check for localtime
        std::tm* tm_ptr = std::localtime(&time);
        if (tm_ptr) {
            std::stringstream ss;
            ss << "[" << std::put_time(tm_ptr, "%H:%M:%S") << "] ";
            std::string ts = ss.str();
            console << ts;
            if (file.is_open()) file << ts;
        }
        atStartOfLine = false;
    }
}

DualStream& DualStream::level(LogLevel l) {
    std::lock_guard<std::mutex> lock(mtx);
    injectTimestamp();

    std::string color, label;
    switch (l) {
        case INFO:    color = BLUE;   label = "[INFO] "; break;
        case SUCCESS: color = GREEN;  label = "[SUCCESS] "; break;
        case WARNING: color = YELLOW; label = "[WARN] "; break;
        case ERROR:   color = RED;    label = "[ERROR] "; break;
    }

    if (isTerminal) console << color << label;
    else console << label;

    if (file.is_open()) file << label;
    return *this;
}

DualStream& DualStream::operator<<(std::ostream& (*manip)(std::ostream&)) {
    std::lock_guard<std::mutex> lock(mtx);
    if (isTerminal) console << RESET;
    manip(console);
    if (file.is_open()) manip(file);
    atStartOfLine = true;
    return *this;
}

// --- Library Hooks ---
// Use the function call to ensure the logger is ready
void __attribute__((constructor)) init_library() {
    get_logger().level(INFO) << "Library initialized via lazy-loading." << std::endl;
}

void __attribute__((destructor)) fini_library() {
    get_logger().level(INFO) << "Library cleanup in progress." << std::endl;
}
