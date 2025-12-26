#include "Logger.hpp"

int main() {

    Logger::setGlobalLevel(DEBUG);

    // 1. Using the Global Logger via Macro
    LOG_SYSTEM(INFO, "Application execution started.");

    // 2. Local Logger for Security/Auth
    Logger authLog("security.log", DEBUG);
    authLog.log(DEBUG, "Checking user permissions for 'guest'...");
    authLog.log(WARNING, "Unauthorized access attempt at resource 0x04.");

    // 3. Local Logger for Hardware/Sensor Data
    Logger sensorLog("sensor.log", INFO);
    sensorLog.log(INFO, "Temperature sensor initialized.");
    sensorLog.log(ERROR, "Sensor timeout: No data received.");

    LOG_SYSTEM(INFO, "Application shutting down cleanly.");
    return 0;
}
