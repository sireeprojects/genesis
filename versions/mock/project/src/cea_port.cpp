#include "cea_internal.h"
#include "cea_port.h"

#include <thread>

namespace cea {

class cea_port::core {
public:
    core(string name);
    ~core();
    void add_stream(cea_stream *stream);
    void add_cmd(cea_stream *stream);
    void exec_cmd(cea_stream *stream);
    void reset();
    string port_name;
    uint32_t port_id;
    vector<cea_stream*> streamq;
    cea_stream *current_stream;
    string msg_prefix;
    thread worker_tid;
    void worker();
    void start_worker();
    void start();
    void stop();
    void pause();
};

cea_port::core::core(string name) {
    // TODO: Implement
}

void cea_port::core::add_stream(cea_stream *stream) {
    // TODO: Implement
}

void cea_port::core::add_cmd(cea_stream *stream) {
    // TODO: Implement
}

void cea_port::core::exec_cmd(cea_stream *stream) {
    // TODO: Implement
}

void cea_port::core::reset() {
    // TODO: Implement
}

void cea_port::core::worker() {
    // TODO: Implement
}

void cea_port::core::start_worker() {
    // TODO: Implement
}

void cea_port::core::start() {
    // TODO: Implement
}

void cea_port::core::stop() {
    // TODO: Implement
}

void cea_port::core::pause() {
    // TODO: Implement
}

cea_port::core::~core() {
    // TODO: Implement
}

// CEA PORT BASE IMPLEMENTATION

cea_port::cea_port(string name) {
    // TODO: Implement
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
