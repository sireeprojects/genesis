#include <iostream>
#include <memory>
using namespace std;

class cea {
public:
    cea();
    void init();
    ~cea();
private:
    class cea_implementation;
    unique_ptr<cea_implementation> impl;
    // cea_implementation *impl;
};

