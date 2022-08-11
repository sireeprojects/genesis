#include <iostream>
#include <string>
using namespace std;

string repeat(const std::string& input, unsigned num)
{
    std::string ret;
    ret.reserve(input.size() * num);
    while (num--)
        ret += input;
    return ret;
}


int main() {
    cout << repeat ("ab cd ef gh ", 10);
    return 0;
}
