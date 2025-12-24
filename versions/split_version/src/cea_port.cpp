#include <thread>
#include <sstream>
#include <iomanip>
#include "cea_field.h"
#include "cea_header.h"
#include "cea_stream.h"
#include "cea_port.h"

namespace cea {

#define CEA_MSG(msg) { \
    stringstream s; \
    s << msg; cealog << "(" << msg_prefix << "|" << setw(8) << left \
    << string(__FUNCTION__) << ")" << ": " <<  s.str() \
    << endl; \
}

uint32_t port_id = 0;

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


cea_port::cea_port(string name) {
    impl = make_unique<core>(name);
}

cea_port::~cea_port() = default;

cea_port::core::core(string name) {
    port_id = cea::port_id;
    cea::port_id++;
    port_name = name + ":" + to_string(port_id);
    reset();
    CEA_MSG("Proxy created with name=" << name << " and id=" << port_id);
}

cea_port::core::~core() = default;

void cea_port::core::reset() {
    msg_prefix = port_name;
}

void cea_port::core::worker() {
    vector<cea_stream*>::iterator it;

    for (it = streamq.begin(); it != streamq.end(); it++) {
        current_stream = *it;
        current_stream->impl->bootstrap_stream();
        current_stream->impl->prepare_for_mutation();
        // current_stream->impl->mutate();
    }
}

void cea_port::core::start_worker() {
    worker_tid = thread(&cea_port::core::worker, this);
    char name[16];
    sprintf(name, "worker_%d", port_id);
    pthread_setname_np(worker_tid.native_handle(), name);
}


void cea_port::add_stream(cea_stream *stream) {
    impl->add_stream(stream);
}

void cea_port::add_cmd(cea_stream *stream) {
    impl->add_cmd(stream);
}

void cea_port::exec_cmd(cea_stream *stream) {
    impl->exec_cmd(stream);
}

void cea_port::core::add_stream(cea_stream *stream) {
    streamq.push_back(stream);
}

void cea_port::core::add_cmd(cea_stream *stream) {
    add_stream(stream);
}

void cea_port::core::exec_cmd(cea_stream *stream) {
// TODO pending implementation
}

void cea_port::core::start() {
    start_worker();
}

void cea_port::core::stop() {
// TODO pending implementation
}

void cea_port::core::pause() {
// TODO pending implementation
}

}
