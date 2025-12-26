#include "cea_internal.h"
#include "cea_header.h"

namespace cea {

class cea_header::core {
public:
    // ctor
    core(cea_header_type hdr_type);

    // dtor
    ~core();

    // Quickly set a fixed value to a field
    void set(cea_field_id id, uint64_t value);

    // Quickly set a fixed pattern to a field (limited set)
    void set(cea_field_id id, string value);

    // Define a complete spec for the generation of a field
    void set(cea_field_id id, cea_field_genspec spec);

    // copy the un-modified field structs from fdb corresponding to the field
    // identifiers that are required by this header
    void build_header_fields();

    void reset();

    // The protocol header type that this class represents
    cea_header_type header_type;

    // A list of field identifiers that is required by this header 
    vector<cea_field_id> field_ids_of_header;

    // A table of field structs that corresponds to the field identifiers
    // required by this header
    vector<cea_field_mutation_spec> header_fields;

    // prefixture to header messages
    string header_name;
    string msg_prefix;
};

cea_header::core::core(cea_header_type hdr_type) {
}

cea_header::core::~core() {
}

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
