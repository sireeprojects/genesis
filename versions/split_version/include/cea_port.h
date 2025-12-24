#include "cea_common.h"

namespace cea {

class cea_port {
public:
    cea_port(string name = "port");
    ~cea_port();
    void add_stream(cea_stream *stream);
    void add_cmd(cea_stream *stream);
    void exec_cmd(cea_stream *stream);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_testbench;
    friend class cea_controller;
};

}
