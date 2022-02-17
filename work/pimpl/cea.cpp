#include "cea.h"
using namespace std;


class cea::cea_implementation {
public:
    cea_implementation();
    void init();
};

cea::cea_implementation::cea_implementation() {
}

void cea::cea_implementation::init() {
    cout << "Welcome to pimpl" << endl;
}

void cea::init() {
    impl->init();
}

// using unique_ptr ------------------------------------------------------------

cea::cea() : impl(new cea_implementation) {
}

cea::~cea() = default;

// using raw pointer -----------------------------------------------------------

// cea::cea() {
//     impl = new cea_implementation;
// }

// cea::~cea() {
//     delete impl;
// }
