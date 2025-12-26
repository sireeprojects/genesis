#include "cea_internal.h"
#include "cea_udf.h"

namespace cea {

class cea_udf::core {
public:
    core();
    ~core();
    void set(cea_field_genspec spec);
    cea_field_mutation_spec field;
    string field_name;
    string msg_prefix;
};

cea_udf::core::core() {
    // TODO: Implement
}

void cea_udf::core::set(cea_field_genspec spec) {
    // TODO: Implement
}

cea_udf::core::~core() {
    // TODO: Implement
}

cea_udf::cea_udf() {
    // TODO: Implement
}

cea_udf::~cea_udf() {
    // TODO: Implement
}

void cea_udf::set(cea_field_genspec spec) {
    // TODO: Implement
}

}
