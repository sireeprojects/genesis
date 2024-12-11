/*
The cleanest way of iterating through a vector is via iterators:

for (auto it = begin (vector); it != end (vector); ++it) {
    it->doSomething ();
}

or (equivalent to the above)

for (auto & element : vector) {
    element.doSomething ();
}
*/
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>
#include <iomanip>
using namespace std;

int main() {

    vector<uint64_t> list = {11,12,13,14,15};

    for (auto it=begin(list); it!=end(list)) {
        cout << *it << endl;
        if (*it == 12) {
            list.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it=begin(list); it!=end(list); it++) {
        cout << *it << endl;
    }

    return 0;
}
