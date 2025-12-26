#include "cea_internal.h"
#include "cea_field.h"

namespace cea {

class cea_field::core {
public:
    // ctor
    core(cea_field_id id);

    // dtor
    ~core();

    // Quickly set a fixed integer value to a field
    void set(uint64_t value);

    // Quickly set a fixed string value to a field
    void set(string value);

    // Define a complete spec for the generation of a field
    void set(cea_field_genspec spec);

    // The protocol field type that this class represents
    cea_field_id field_id;

    // A table of field structs that corresponds to the field identifiers
    // required by this field
    cea_field_mutation_spec field;

    // prefixture to field messages
    string field_name;
    string msg_prefix;
};

cea_field::core::core(cea_field_id id) {
    // TODO: Implement
}

cea_field::core::~core() {
    // TODO: Implement
}

cea_field::cea_field(cea_field_id id) {
    // TODO: Implement
}

cea_field::~cea_field() {
    // TODO: Implement
}

void cea_field::set(uint64_t value) {
    // TODO: Implement
}

void cea_field::set(string value) {
    // TODO: Implement
}

void cea_field::set(cea_field_genspec spec) {
    // TODO: Implement
}

}
