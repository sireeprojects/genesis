#include "cea_internal.h"
#include "cea_header.h"

namespace cea {

class cea_header::core {
public:
    core(cea_header_type hdr_type);
    ~core();
    void set(cea_field_id id, uint64_t value);
    void set(cea_field_id id, string value);
    void set(cea_field_id id, cea_field_genspec spec);
    void build_header_fields();
    void reset();
    cea_header_type header_type;
    vector<cea_field_id> field_ids_of_header;
    vector<cea_field_mutation_spec> header_fields;
    string header_name;
    string msg_prefix;
};

cea_header::core::core(cea_header_type hdr_type) {
	 // TODO: Implement
}

void cea_header::core::set(cea_field_id id, uint64_t value) {
	 // TODO: Implement
};

void cea_header::core::set(cea_field_id id, string value) {
	 // TODO: Implement
};

void cea_header::core::set(cea_field_id id, cea_field_genspec spec) {
	 // TODO: Implement
};

void cea_header::core::build_header_fields() {
	 // TODO: Implement
};

void cea_header::core::reset() {
	 // TODO: Implement
};

cea_header::core::~core() {
	 // TODO: Implement
}

// CEA HEADER BASE IMPLEMENTATION

cea_header::cea_header(cea_header_type hdr_type) {
    // TODO: Implement
}

cea_header::~cea_header() {
    // TODO: Implement
}

void cea_header::set(cea_field_id id, uint64_t value) {
    // TODO: Implement
}

void cea_header::set(cea_field_id id, string value) {
    // TODO: Implement
}

void cea_header::set(cea_field_id id, cea_field_genspec spec) {
    // TODO: Implement
}

}
