#include "cea.h"
using namespace std;


class cea::core {
public:
    core();
    void init();
};

cea::core::core() {
}

void cea::core::init() {
    cout << "CEA: Welcome to pimpl" << endl;
}

void cea::init() {
    impl->init();
}

// using unique_ptr ------------------------------------------------------------

cea::cea() : impl(new core) {
}

cea::~cea() = default;

// using raw pointer -----------------------------------------------------------

// cea::cea() {
//     impl = new core;
// }

// cea::~cea() {
//     delete impl;
// }


class proxy::core {
public:
    core();
    void init();
};

proxy::core::core() {
}

void proxy::core::init() {
    cout << "PXY: Welcome to pimpl" << endl;
}

proxy::proxy() : impl(new core) {
}

proxy::~proxy() = default;

void proxy::init() {
    impl->init();
}

