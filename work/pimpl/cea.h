#include <iostream>
#include <memory>
using namespace std;

class cea {
public:
    cea();
    void init();
    ~cea();
private:
    class core;
    unique_ptr<core> impl;
    // core *impl;
};

class proxy {
public:
    proxy();
    void init();
    ~proxy();
private:
    class core;
    unique_ptr<core> impl;
    // core *impl;
};
