#include "cea_udf.h"

namespace cea {

class cea_udf::core {
public:
    // ctor
    core();

    // dtor
    ~core();

    // Define a complete spec for the generation of a field
    void set(cea_field_genspec spec);

    // A table of field structs that corresponds to the field identifiers
    // required by this field
    cea_field_mutation_spec field;

    // prefixture to field messages
    string field_name;
    string msg_prefix;
};

cea_udf::cea_udf() {
    impl = make_unique<core>(); 
}

cea_udf::~cea_udf() = default;

cea_udf::core::core() {
    field = {};
}

cea_udf::core::~core() = default;

void cea_udf::set(cea_field_genspec spec) {
    impl->set(spec);
}

void cea_udf::core::set(cea_field_genspec spec) {
    field.gspec = spec;
}

}
