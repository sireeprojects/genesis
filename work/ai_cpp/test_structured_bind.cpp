#include <iostream>
#include <tuple>
#include <string>
#include <map>

std::tuple<std::string, int, bool> get_user_data() {
    return {"Alice", 25, true};
}

int main() {
    // Old way: std::get<0>(data) or std::tie
    // New way:
    auto [name, age, isActive] = get_user_data();
    std::cout << name << " is " << age << " years old." << std::endl;

    std::map<std::string, int> inventory = {{"Apples", 10}, {"Bananas", 5}, {"Pears", 20}};

    // Unpacking the key and value directly in the loop
    for (const auto& [item, count] : inventory) {
        std::cout << item << ": " << count << std::endl;
    }
    return 0;
}
