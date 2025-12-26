#pragma once

namespace cea {

class cea_field {
public:
    cea_field(cea_field_id id);
    ~cea_field();
    void set(uint64_t value);
    void set(string value);
    void set(cea_field_genspec spec);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_stream;
};

}
