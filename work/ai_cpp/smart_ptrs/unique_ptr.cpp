#include <iostream>
#include <memory>

class Laptop {
public:
    Laptop() { std::cout << "Laptop bought!\n"; }
    ~Laptop() { std::cout << "Laptop recycled!\n"; } // Destructor
    void browse() { std::cout << "Browsing web...\n"; }
};

int main() {
    // 'pete' owns this Laptop exclusively
    std::unique_ptr<Laptop> pete = std::make_unique<Laptop>();

    pete->browse();

    // You cannot copy a unique_ptr! 
    // std::unique_ptr<Laptop> sarah = pete; // ERROR! Pete won't share.
    
} // <--- Pete goes out of scope here. The Laptop is automatically recycled.
