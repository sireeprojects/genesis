#include <iostream>
using namespace std;

void addFive(int& num) {
    num += 5;
}

class Robot {
public:
    std::string name;
    int batteryLevel;

    Robot(std::string n, int b) : name(n), batteryLevel(b) {}

    void status() {
        std::cout << name << " battery: " << batteryLevel << "%" << std::endl;
    }
};

int main() {
    int fruit = 10;
    int &ref = fruit; // ref is now a nickname of fruit

    cout << fruit << endl;
    cout << ref << endl;

    addFive(fruit); // passing regular variable
    cout << fruit << endl;

    addFive(ref); // passing reference
    cout << fruit << endl;

    Robot r = Robot("irobot", 100);

    return 0;
}
