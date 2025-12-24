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

cea_testbench::cea_testbench() {
    impl = make_unique<core>();
}

cea_testbench::~cea_testbench() = default;

cea_testbench::core::core() {
}

cea_testbench::core::~core() = default;

void cea_testbench::add_port(cea_port *port) {
    impl->add_port(port);
}

void cea_testbench::add_stream(cea_stream *stream, cea_port *port) {
    impl->add_stream(stream, port);
}

void cea_testbench::add_cmd(cea_stream *stream, cea_port *port) {
    impl->add_cmd(stream, port);
}

void cea_testbench::exec_cmd(cea_stream *stream, cea_port *port) {
    impl->exec_cmd(stream, port);
}

void cea_testbench::core::add_port(cea_port *port) {
    ports.push_back(port);
    controller.gports[controller.port_cntr] = port;
    controller.port_cntr++;
}

void cea_testbench::core::add_stream(cea_stream *stream, cea_port *port) {
    if (ports.size() == 0) {
        CEA_ERR_MSG("Cannot add stream to port since no ports are added to the testbench");
    }
    if (port != NULL) {
        vector<cea_port*>::iterator it;
        for (it = ports.begin(); it != ports.end(); it++) {
            if ((*it)->impl->port_id == port->impl->port_id) {
                uint32_t idx = distance(ports.begin(), it);
                ports[idx]->add_stream(stream);
            }
        }
    } else {
        for (uint32_t idx=0; idx<ports.size(); idx++) {
            ports[idx]->add_stream(stream);
        }
    }
}

void cea_testbench::core::add_cmd(cea_stream *stream, cea_port *port) {
    add_stream(stream, port);
}

void cea_testbench::core::exec_cmd(cea_stream *stream, cea_port *port) {
// TODO pending implementation
}

void cea_testbench::start(cea_port *port) {
    impl->start(port);
}

void cea_testbench::stop(cea_port *port) {
    impl->stop(port);
}

void cea_testbench::pause(cea_port *port) {
    impl->pause(port);
}

void cea_testbench::core::start(cea_port *port) {
    if (port != NULL) {
        vector<cea_port*>::iterator it;

        // start threads
        for (it = ports.begin(); it != ports.end(); it++) {
            if ((*it)->impl->port_id == port->impl->port_id) {
                uint32_t idx = distance(ports.begin(), it);
                ports[idx]->impl->start();
            }
        }
        // join threads
        for (it = ports.begin(); it != ports.end(); it++) {
            if ((*it)->impl->port_id == port->impl->port_id) {
                uint32_t idx = distance(ports.begin(), it);
                ports[idx]->impl->worker_tid.join();
            }
        }
    } else {
        // start threads
        for (uint32_t idx=0; idx<ports.size(); idx++) {
            ports[idx]->impl->start();
        }
        // join threads
        for (uint32_t idx=0; idx<ports.size(); idx++) {
            ports[idx]->impl->worker_tid.join();
        }
    }
}

void cea_testbench::core::stop(cea_port *port) {
    if (port != NULL) {
        vector<cea_port*>::iterator it;
        for (it = ports.begin(); it != ports.end(); it++) {
            if ((*it)->impl->port_id == port->impl->port_id) {
                uint32_t idx = distance(ports.begin(), it);
                ports[idx]->impl->stop();
            }
        }
    } else {
        for (uint32_t idx=0; idx<ports.size(); idx++) {
            ports[idx]->impl->stop();
        }
    }
}

void cea_testbench::core::pause(cea_port *port) {
    if (port != NULL) {
        vector<cea_port*>::iterator it;
        for (it = ports.begin(); it != ports.end(); it++) {
            if ((*it)->impl->port_id == port->impl->port_id) {
                uint32_t idx = distance(ports.begin(), it);
                ports[idx]->impl->pause();
            }
        }
    } else {
        for (uint32_t idx=0; idx<ports.size(); idx++) {
            ports[idx]->impl->pause();
        }
    }
}
 
}
