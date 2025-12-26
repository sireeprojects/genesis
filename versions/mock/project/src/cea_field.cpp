#include "cea_internal.h"
#include "cea_field.h"

namespace cea {

class cea_field::core {
public:
    core(cea_field_id id);
    ~core();
    void set(uint64_t value);
    void set(string value);
    void set(cea_field_genspec spec);
    cea_field_id field_id;
    cea_field_mutation_spec field;
    string field_name;
    string msg_prefix;
};

cea_field::core::core(cea_field_id id) {
    // TODO: Implement
}

void cea_field::core::set(uint64_t value) {
	 // TODO: Implement
};

void cea_field::core::set(string value) {
	 // TODO: Implement
};

void cea_field::core::set(cea_field_genspec spec) {
	 // TODO: Implement
};

cea_field::core::~core() {
    // TODO: Implement
}

// CEA FIELD BASE IMPLEMENTATION

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
