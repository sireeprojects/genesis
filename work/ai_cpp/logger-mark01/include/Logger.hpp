#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>

enum LogLevel { DEBUG, INFO, WARNING, ERROR };

class Logger {
public:
    Logger(const std::string& filename, LogLevel localThreshold);
    ~Logger();

    static Logger& getGlobal();
    
    // Declaration
    static void setGlobalLevel(LogLevel level);

    void log(LogLevel level, const std::string& message);

private:
    std::ofstream logFile;
    LogLevel threshold;
    std::mutex mtx;

    static LogLevel globalMinLevel;

    std::string getLevelString(LogLevel level);
    std::string formatMessage(LogLevel level, const std::string& message);
};

#define LOG_SYSTEM(level, msg) Logger::getGlobal().log(level, msg)

#endif
