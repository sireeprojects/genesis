#include "cea_common.h"

namespace cea {

class cea_header {
public:
    cea_header(cea_header_type hdr_type);
    ~cea_header();
    void set(cea_field_id id, uint64_t value);
    void set(cea_field_id id, string value);
    void set(cea_field_id id, cea_field_genspec spec);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_stream;
    friend class cea_controller;
};

}
