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

cea_field::cea_field(cea_field_id id) {
    impl = make_unique<core>(id); 
}

cea_field::~cea_field() = default;

cea_field::core::core(cea_field_id id) {
    field_id = id;
    field = get_field(mtable, id);
}

cea_field::core::~core() = default;

void cea_field::set(uint64_t value) {
    impl->set(value);
}

void cea_field::set(string value) {
    impl->set(value);
}

void cea_field::set(cea_field_genspec spec) {
    impl->set(spec);
}

void cea_field::core::set(uint64_t value) {
    if (field.defaults.type != Integer) {
        CEA_ERR_MSG("The field "
        << cea_trim(field.defaults.name) << " accepts only string patterns");
        abort();
    }
    field.gspec.nmr.value = value;
    field.gspec.gen_type = Fixed_Value;
}

void cea_field::core::set(string value) {
    if (field.defaults.type == Integer) {
        CEA_ERR_MSG("The field "
        << cea_trim(field.defaults.name) << " accepts only integer values");
        abort();
    }
    field.gspec.str.value = value;
    field.gspec.gen_type = Fixed_Value;
}

void cea_field::core::set(cea_field_genspec spec) {
    field.gspec = spec;
}

}
