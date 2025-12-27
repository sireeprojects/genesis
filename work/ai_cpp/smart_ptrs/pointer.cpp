#include <iostream>
using namespace std;

int main() {
    int fruit = 10;
    int *ptr = &fruit; // ptr holds address of fruit

    cout << fruit << endl;
    cout << *ptr << endl;

    cout << ptr << endl;

    return 0;
}
