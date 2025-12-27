#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <atomic>
#include <thread>
#include <chrono>
#include <iomanip>
#include <stdexcept>

// --- 1. NETWORKING DATA STRUCTURES ---
struct EthernetHeader {
    uint8_t destMac[6];
    uint8_t srcMac[6];
    uint16_t etherType;
};

class Frame {
public:
    EthernetHeader header;
    std::vector<uint8_t> payload; // Dynamic payload via vector

    static std::string macToString(const uint8_t* mac) {
        std::stringstream ss;
        for (int i = 0; i < 6; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)mac[i] << (i < 5 ? ":" : "");
        }
        return ss.str();
    }
};

// --- 2. THE PORT CLASS ---
class Port {
public:
    std::string id;
    std::atomic<uint64_t> txCount{0};
    std::atomic<uint64_t> byteCount{0};
    std::atomic<double> targetFps{100.0}; // Default rate

    Port(std::string name) : id(name) {}

    // Pass by Const Reference for speed
    void transmit(const Frame& f) {
        // Simple Rate Limiter Logic
        auto interval = std::chrono::microseconds(static_cast<long long>(1000000.0 / targetFps.load()));
        static thread_local auto lastPacketTime = std::chrono::steady_clock::now();
        
        auto now = std::chrono::steady_clock::now();
        if (now - lastPacketTime < interval) {
            std::this_thread::sleep_for(interval - (now - lastPacketTime));
        }
        lastPacketTime = std::chrono::steady_clock::now();

        // Perform "Work"
        txCount.fetch_add(1, std::memory_order_relaxed);
        byteCount.fetch_add(sizeof(EthernetHeader) + f.payload.size(), std::memory_order_relaxed);
    }

    void showStats() const {
        std::cout << "Port: " << std::left << std::setw(10) << id 
                  << " | Pkts: " << std::setw(8) << txCount.load() 
                  << " | Bytes: " << byteCount.load() << "\n";
    }
};

// --- 3. THE MANAGER (OWNER) ---
class PortManager {
private:
    // Unique ownership of ports in a map
    std::map<std::string, std::unique_ptr<Port>> portMap;

public:
    void addPort(const std::string& name) {
        portMap[name] = std::make_unique<Port>(name);
    }

    // Return reference for work
    Port& getPort(const std::string& name) {
        if (portMap.find(name) == portMap.end()) 
            throw std::runtime_error("Port not found: " + name);
        return *(portMap[name]);
    }

    void displayAllStats() const {
        std::cout << "\n--- CURRENT SYSTEM STATS ---\n";
        for (auto const& [name, ptr] : portMap) {
            ptr->showStats();
        }
    }
};

// --- 4. THE GENERATOR LOOP ---
void generatorWorker(Port& p, const Frame& f) {
    std::cout << "Starting traffic on " << p.id << "...\n";
    // Send 500 packets as a test
    for(int i = 0; i < 500; ++i) {
        p.transmit(f); 
    }
}

int main() {
    try {
        PortManager manager;
        manager.addPort("ETH_0");
        manager.addPort("ETH_1");

        // Prepare a test frame
        uint8_t dst[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast
        uint8_t src[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
        Frame testFrame;
        std::copy(dst, dst+6, testFrame.header.destMac);
        std::copy(src, src+6, testFrame.header.srcMac);
        testFrame.header.etherType = 0x0800;
        testFrame.payload.resize(1024, 0xAB); // 1KB Payload

        // Borrow references to ports
        Port& p0 = manager.getPort("ETH_0");
        Port& p1 = manager.getPort("ETH_1");
        
        p0.targetFps = 1000.0; // High speed
        p1.targetFps = 500.0;  // Lower speed

        // Start threads (Pass by Reference using std::ref)
        std::thread t1(generatorWorker, std::ref(p0), std::ref(testFrame));
        std::thread t2(generatorWorker, std::ref(p1), std::ref(testFrame));

        // Monitor while threads are running
        for(int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            manager.displayAllStats();
        }

        t1.join();
        t2.join();
        
        std::cout << "\nFinal Report:";
        manager.displayAllStats();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
