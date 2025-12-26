#pragma once

namespace cea {

class cea_udf {
public:
    cea_udf();
    ~cea_udf();
    void set(cea_field_genspec spec);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_stream;
    friend class cea_controller;
};

}
