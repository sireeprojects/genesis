#include "cea_internal.h"
#include "cea_port.h"

#include <thread>

namespace cea {

class cea_port::core {
public:
    // ctor
    core(string name);

    // dtor
    ~core();

    // add stream to port queue
    void add_stream(cea_stream *stream);

    // add a command (in the form of cea_stream) to port queue
    void add_cmd(cea_stream *stream);

    // execute a command immediately does not add to port queue
    void exec_cmd(cea_stream *stream);

    // set default values
    void reset();

    // set when the port object is created
    string port_name;

    // automatically assigned when the port object is created
    // the value of the field is set from the global variable port_id
    uint32_t port_id;

    // user's test streams will be pushed into this queue (container1)
    vector<cea_stream*> streamq;

    // handle to the stream being processed
    cea_stream *current_stream;

    // prefixture to all msgs from this port
    string msg_prefix;

    // main port thread
    thread worker_tid;
    void worker();
    void start_worker();

    // execution control
    void start();
    void stop();
    void pause();
};

cea_port::core::core(string name) {
}

cea_port::core::~core() {
}

cea_port::cea_port(string name) {
}

cea_port::~cea_port() {
    // TODO: Implement
}

void cea_port::add_stream(cea_stream *stream) {
    // TODO: Implement
}

void cea_port::add_cmd(cea_stream *stream) {
    // TODO: Implement
}

void cea_port::exec_cmd(cea_stream *stream) {
    // TODO: Implement
}


}
