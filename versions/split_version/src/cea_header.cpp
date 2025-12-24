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

cea_header::cea_header(cea_header_type hdr_type) {
    impl = make_unique<core>(hdr_type); 
}

cea_header::~cea_header() = default;

void cea_header::set(cea_field_id id, uint64_t value) {
    impl->set(id, value);
}

void cea_header::set(cea_field_id id, cea_field_genspec spec) {
    impl->set(id, spec);
}

void cea_header::set(cea_field_id id, string value) {
    impl->set(id, value);
}

cea_header::core::core(cea_header_type hdr_type) {
    header_name = string("Header") + ":" + cea_header_name[hdr_type];
    msg_prefix = header_name;
    header_type = hdr_type;
    build_header_fields();
}

cea_header::core::~core() = default;

void cea_header::core::set(cea_field_id id, string value) {
    // try to extract the field represented by id
    auto field = find_if(header_fields.begin(), header_fields.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    if (field != header_fields.end()) { // if field id is valid
        // abort if field type is not a pattern
        if (field->defaults.type == Integer) {
            CEA_ERR_MSG("The field "
            << cea_trim(mtable[id].defaults.name) << " accepts only integer values");
            abort();
        }
        // validate input
        switch(field->defaults.type) {
            case Pattern_MAC: {
                if(!regex_match(value, regex_mac)) {
                    CEA_ERR_MSG("The value " << value << 
                    " does not match the acceptable pattern for " 
                    << cea_trim(mtable[id].defaults.name));
                    abort();
                }
                break;
            } 
            case Pattern_IPv4: {
                if(!regex_match(value, regex_ipv4)) {
                    CEA_ERR_MSG("The value " << value << 
                    " does not match the acceptable pattern for " 
                    << cea_trim(mtable[id].defaults.name));
                    abort();
                }
                break;
            } 
            case Pattern_IPv6: {
                if(!regex_match(value, regex_ipv6)) {
                    CEA_ERR_MSG("The value " << value << 
                    " does not match the acceptable pattern for " 
                    << cea_trim(mtable[id].defaults.name));
                    abort();
                }
                break;
            } 
            default:{
                CEA_ERR_MSG("Input validation error for the field: "
                << cea_trim(mtable[id].defaults.name));
                abort();
            }
        }
        // accept user value
        field->gspec.gen_type = Fixed_Value;
        field->gspec.str.value = value;
        field->mdata.is_mutable = true;
    } else {
        CEA_ERR_MSG("The field "
        << cea_trim(mtable[id].defaults.name) << " does not belong to the "
        << cea_header_name[header_type] << " header");
        abort();
    }
}

void cea_header::core::set(cea_field_id id, uint64_t value) {
    // try to extract the field represented by id
    auto field = find_if(header_fields.begin(), header_fields.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    if (field != header_fields.end()) { // if field id is valid
        // abort if field type is not a integer
        if (field->defaults.type != Integer) {
            CEA_ERR_MSG("The field "
            << cea_trim(mtable[id].defaults.name) << " accepts only string patterns");
            abort();
        }
        // accept user value
        field->gspec.gen_type = Fixed_Value;
        field->gspec.nmr.value = value;
        field->mdata.is_mutable = true;
    } else {
        CEA_ERR_MSG("The field "
        << cea_trim(mtable[id].defaults.name) << " does not belong to the "
        << cea_header_name[header_type] << " header");
        abort();
    }
}

void cea_header::core::set(cea_field_id id, cea_field_genspec spec) {
    auto field = find_if(header_fields.begin(), header_fields.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    if (field != header_fields.end()) {
        // TODO Is it possible to validate spec before assigning to gspec?
        field->gspec = spec; // TODO check if vectors also get copied
        field->mdata.is_mutable = true;
    } else {
        CEA_ERR_MSG("The field "
        << cea_trim(mtable[id].defaults.name) << " does not belong to the "
        << cea_header_name[header_type] << " header");
        abort();
    }
}

void cea_header::core::build_header_fields() {
    // TODO are the clear() ok in multi-reset/multi-iteration scenario?
    // TODO what if user re-configures the header fields from test and starts
    //      the test again
    field_ids_of_header.clear();
    header_fields.clear();

    // extract the list of field ids that make up this header
    field_ids_of_header = header_to_field_map[header_type];

    for (auto id : field_ids_of_header) {
        auto item = get_field(mtable, id);
        header_fields.push_back(item);
    }
}

// TODO
void cea_header::core::reset() {
}

}
