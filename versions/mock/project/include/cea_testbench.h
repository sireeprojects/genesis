#pragma once

namespace cea {

class cea_port;
class cea_stream;

class cea_testbench {
public:
    cea_testbench(); // TODO add socket port number for external interfacing
    ~cea_testbench();
    void add_port(cea_port *port);
    void add_stream(cea_stream *stream, cea_port *port = NULL);
    void add_cmd(cea_stream *stream, cea_port *port = NULL);
    void exec_cmd(cea_stream *stream, cea_port *port = NULL);
    void start(cea_port *port = NULL);
    void stop(cea_port *port = NULL);
    void pause(cea_port *port = NULL);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_controller;
};

}
