#include "cea_internal.h"
#include "cea_stream.h"

namespace cea {

#define CEA_MAX_RND_ARRAYS 16

typedef enum  {
    NEW_FRAME,
    TRANSMIT
} mutation_states;

class cea_stream::core {
public:
    // ctor
    core(string name);

    // dtor
    ~core();

    // Quickly set a fixed value to a property
    void set(cea_field_id id, uint64_t value);

    // Define a complete spec for the generation of a property
    void set(cea_field_id id, cea_field_genspec spec);

    // enable or disable a stream feature
    void set(cea_stream_feature_id feature, bool mode);

    // Based on user specification of the frame, build a vector of 
    // field ids in the sequence required by the specification
    void collate_frame_fields();

    // The addition of mpls/vlan/llc/snap affects the position of ethertype and
    // length fields. update/insert type or len after arranging all fields in
    // the required sequence
    // TODO check if this is required
    void update_ethertype_and_len();

    // evaluate field length and calculate offset of all fields of this stream
    void build_field_offsets();

    // build cseq by parsing fseq and adding only mutable fields
    void filter_mutable_fields();

    // concatenate all fields reuired by the frame spec
    uint32_t splice_frame_fields(unsigned char *buf);

    // build size and payload pattern arrays
    void build_payload_arrays();

    void prepare_genspec();

    void build_runtime();

    void build_principal_frame();

    // process the headers and fields and prepare for generation
    void bootstrap_stream();

    // begin generation
    void mutate();

    // print the frame structure, mutable fields and stream properties
    void print_stream();

    // print a vector of field specs
    void print_fields(vector<cea_field_mutation_spec> field_group);

    // Factory reset of the stream core
    void reset();

    // Store the header pointers added to the stream
    // for generation
    vector<cea_header*> frame_headers;
    
    // frame_fields will be used to store all the fields added
    // by user by the way of adding headers
    vector<cea_field_mutation_spec> frame_fields;

    // store stream properties
    vector<cea_field_mutation_spec> stream_properties;
    
    // initialize stream to its default values
    void init_stream_properties();

    // store user defined fields
    vector<cea_field_spec> udfs;

    // mutable_fields will be used to store only those fields that will used during
    // stream generation
    vector<cea_field_mutation_spec> mutable_fields;

    // at the end of the mutation logic, the mutables will be empty
    // so if the user press start button again, the mutables will be empty
    // so a bkp version is maintained to restore the content of mutables
    // TODO check if this backup is required because we can simply call
    //      filter_mutable_fields() to regenerate the mutable fields
    vector<cea_field_mutation_spec> mutable_fields_clone;

    // functions used during mutation of strings
    void convert_string_to_uca(string address, unsigned char *op);
    void convert_mac_to_uca(string address, unsigned char *op);
    void convert_ipv4_to_uca(string address, unsigned char *op);
    void convert_ipv6_to_uca(string address, unsigned char *op);
    unsigned char convert_char_to_int(string hexNumber);
    int convert_nibble_to_int(char digit);

    string convert_int_to_ipv4(uint64_t ipAddress);
    uint64_t convert_string_ipv4_internal(string addr);

    // pcap handle for recording
    // pcap *txpcap;
    // pcap *rxpcap;

    // Prefixture to stream messages
    string stream_name;
    uint32_t stream_id;
    string msg_prefix;

    uint32_t hdr_len; // TODO check and rename to match intent
    uint32_t nof_sizes;
    uint32_t hdr_size;
    uint32_t meta_size;
    uint32_t crc_len;
    vector<uint32_t> vof_frame_sizes;
    vector<uint32_t> vof_computed_frame_sizes;
    vector<uint32_t> vof_payload_sizes;
    unsigned char *arof_payload_data;
    unsigned char *arof_rnd_payload_data[CEA_MAX_RND_ARRAYS];

    // TODO what are these
    unsigned char *payload_pattern;
    uint32_t payload_pattern_size;

    // principal frame
    unsigned char *pf;
    unsigned char test_buffer[512];

    // random
    random_device rd;

    // GSFM //
    void prepare_for_mutation();
    void mutate_next_frame();
    int mutate_enqueue(uint32_t space);
    vector<cea_field_mutation_spec> mut;
    cea_field_genspec lenspec;

    uint32_t num_txns;
    uint32_t num_txns_transmitted;
    uint32_t offset;
    uint32_t num_elems;
    uint32_t num_elems_transmitted;
    bool txdone;
    bool stream_done;
    mutation_states state;
};

cea_stream::core::core(string name) {
}

cea_stream::core::~core() {
}

cea_stream::cea_stream(string name) {
}

cea_stream::~cea_stream() {
    // TODO: Implement
}

void cea_stream::set(cea_field_id id, uint64_t value) {
    // TODO: Implement
}

void cea_stream::set(cea_field_id id, cea_field_genspec spec) {
    // TODO: Implement
}

void cea_stream::set(cea_stream_feature_id feature, bool mode) {
    // TODO: Implement
}

void cea_stream::add_header(cea_header *header) {
    // TODO: Implement
}

void cea_stream::add_udf(cea_field *field) {
    // TODO: Implement
}

}
