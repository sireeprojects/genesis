#include "Logger.hpp"
#include <ctime>
#include <iomanip>

// 1. Define the static variable
LogLevel Logger::globalMinLevel = INFO;

// 2. Define the static method (This fixes your error)
void Logger::setGlobalLevel(LogLevel level) {
    globalMinLevel = level;
}

Logger::Logger(const std::string& filename, LogLevel localThreshold) 
    : threshold(localThreshold) {
    logFile.open(filename, std::ios::app);
}

Logger::~Logger() {
    if (logFile.is_open()) logFile.close();
}

Logger& Logger::getGlobal() {
    static Logger instance("global_system.log", INFO);
    return instance;
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < globalMinLevel) return;
    if (level < threshold) return;

    std::lock_guard<std::mutex> lock(mtx);
    std::string formatted = formatMessage(level, message);

    std::cout << formatted << std::endl;
    if (logFile.is_open()) {
        logFile << formatted << std::endl;
    }
}

std::string Logger::getLevelString(LogLevel level) {
    switch (level) {
        case DEBUG:   return "DEBUG";
        case INFO:    return "INFO";
        case WARNING: return "WARN";
        case ERROR:   return "ERR";
        default:      return "LOG";
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& message) {
    std::time_t now = std::time(nullptr);
    std::tm* ltm = std::localtime(&now);
    char timeBuffer[20];
    std::strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S", ltm);
    return "[" + std::string(timeBuffer) + "] [" + getLevelString(level) + "] " + message;
}
