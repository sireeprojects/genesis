#include "cea_internal.h"
#include "cea_testbench.h"

namespace cea {

class cea_testbench::core {
public:
    core();
    ~core();
    void add_port(cea_port *port);
    void add_stream(cea_stream *stream, cea_port *port=NULL);
    void add_cmd(cea_stream *stream, cea_port *port=NULL);
    void exec_cmd(cea_stream *stream, cea_port *port=NULL);
    void start(cea_port *port = NULL);
    void stop(cea_port *port = NULL);
    void pause(cea_port *port = NULL);
    vector<cea_port*> ports;
    string msg_prefix;
};

cea_testbench::core::core() {
    // TODO: Implement
}

void cea_testbench::core::add_port(cea_port *port) {
    // TODO: Implement
}

void cea_testbench::core::add_stream(cea_stream *stream, cea_port *port) {
    // TODO: Implement
}

void cea_testbench::core::add_cmd(cea_stream *stream, cea_port *port) {
    // TODO: Implement
}

void cea_testbench::core::exec_cmd(cea_stream *stream, cea_port *port) {
    // TODO: Implement
}

void cea_testbench::core::start(cea_port *port) {
    // TODO: Implement
}

void cea_testbench::core::stop(cea_port *port) {
    // TODO: Implement
}

void cea_testbench::core::pause(cea_port *port) {
    // TODO: Implement
}

cea_testbench::core::~core() {
    // TODO: Implement
}

// CEA TESTBENCH BASE IMPLEMENTATION

cea_testbench::cea_testbench() {
    // TODO: Implement
}

cea_testbench::~cea_testbench() {
    // TODO: Implement
}

void cea_testbench::add_port(cea_port *port) {
    // TODO: Implement
}

void cea_testbench::add_stream(cea_stream *stream, cea_port *port) {
    // TODO: Implement
}

void cea_testbench::add_cmd(cea_stream *stream, cea_port *port) {
    // TODO: Implement
}

void cea_testbench::exec_cmd(cea_stream *stream, cea_port *port) {
    // TODO: Implement
}

void cea_testbench::start(cea_port *port) {
    // TODO: Implement
}

void cea_testbench::stop(cea_port *port) {
    // TODO: Implement
}

void cea_testbench::pause(cea_port *port) {
    // TODO: Implement
}

}
