#include <iostream>
#include <memory> // unique_ptr
using namespace std;

class cea_port {
public:
    cea_port();
    void set();
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_friend;
};

class cea_stream {
public:
    cea_stream();
    void set();
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_friend;
};

//------------------------------------------------------------------------------
class cea_port::core {
public:
    core() { cout << "Creating cea_port::core" << endl; }
    void set() { cout << "cea_port::core-> set() called" << endl; }
    cea_stream s;
};

cea_port::cea_port() {
    cout << "Creating cea_port" << endl;
    impl = make_unique<core>(); 
}

void cea_port::set() {
    impl->set();
}
//-----------------------------------
class cea_stream::core {
public:    
    core() {cout << "Creating cea_stream::core"<< endl;}
    void set() { cout << "cea_stream::core-> set() called" << endl; }
};

cea_stream::cea_stream() {
    cout << "Creating cea_stream" << endl;
    impl = make_unique<core>(); 
}

void cea_stream::set() {
    impl->set();
}

class cea_friend {
public:
    cea_friend() {
        cout << "Creating cea_friend" << endl;
    }
    void access_port(cea_port *p, cea_stream *s) {
        p->impl->set();
        s->impl->set();
    }

    void access_port(cea_port *p) {
        cout << "---------" << endl;
        p->impl->s.set();
        p->impl->s.impl->set();
    }
};

//------------------------------------------------------------------------------

int main() {
    cea_port p;
    p.set();

    cea_stream s;
    s.set();

    cea_friend f;
    // f.access_port(&p, &s);
    f.access_port(&p);
    return 0;
}
