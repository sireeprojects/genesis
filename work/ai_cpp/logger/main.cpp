#include "Logger.h"

int main() {
    LOG.level(INFO)    << "Application starting up..." << std::endl;
    LOG.level(SUCCESS) << "Configuration loaded." << std::endl;
    LOG.level(WARNING) << "Disk cache is nearly full." << std::endl;
    LOG.level(ERROR)   << "Database connection timed out!" << std::endl;
    
    LOG << "This is a standard log without a specific level." << std::endl;

    return 0;
}
