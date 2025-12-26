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
    core(string name);
    ~core();
    void set(cea_field_id id, uint64_t value);
    void set(cea_field_id id, cea_field_genspec spec);
    void set(cea_stream_feature_id feature, bool mode);
    void collate_frame_fields();
    void update_ethertype_and_len();
    void build_field_offsets();
    void filter_mutable_fields();
    uint32_t splice_frame_fields(unsigned char *buf);
    void build_payload_arrays();
    void prepare_genspec();
    void build_runtime();
    void build_principal_frame();
    void bootstrap_stream();
    void mutate();
    void print_stream();
    void print_fields(vector<cea_field_mutation_spec> field_group);
    void reset();
    vector<cea_header*> frame_headers;
    vector<cea_field_mutation_spec> frame_fields;
    vector<cea_field_mutation_spec> stream_properties;
    void init_stream_properties();
    vector<cea_field_spec> udfs;
    vector<cea_field_mutation_spec> mutable_fields;
    vector<cea_field_mutation_spec> mutable_fields_clone;
    void convert_string_to_uca(string address, unsigned char *op);
    void convert_mac_to_uca(string address, unsigned char *op);
    void convert_ipv4_to_uca(string address, unsigned char *op);
    void convert_ipv6_to_uca(string address, unsigned char *op);
    unsigned char convert_char_to_int(string hexNumber);
    int convert_nibble_to_int(char digit);
    string convert_int_to_ipv4(uint64_t ipAddress);
    uint64_t convert_string_ipv4_internal(string addr);
    string stream_name;
    uint32_t stream_id;
    string msg_prefix;
    uint32_t hdr_len;
    uint32_t nof_sizes;
    uint32_t hdr_size;
    uint32_t meta_size;
    uint32_t crc_len;
    vector<uint32_t> vof_frame_sizes;
    vector<uint32_t> vof_computed_frame_sizes;
    vector<uint32_t> vof_payload_sizes;
    unsigned char *arof_payload_data;
    unsigned char *arof_rnd_payload_data[CEA_MAX_RND_ARRAYS];
    unsigned char *payload_pattern;
    uint32_t payload_pattern_size;
    unsigned char *pf;
    unsigned char test_buffer[512];
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
    // pcap handle for recording
    // pcap *txpcap;
    // pcap *rxpcap;
};

cea_stream::core::core(string name) {
    // TODO: Implement
}

void cea_stream::core::set(cea_field_id id, uint64_t value) {
	 // TODO: Implement
}

void cea_stream::core::set(cea_field_id id, cea_field_genspec spec) {
	 // TODO: Implement
}

void cea_stream::core::set(cea_stream_feature_id feature, bool mode) {
	 // TODO: Implement
}

void cea_stream::core::collate_frame_fields() {
	 // TODO: Implement
}

void cea_stream::core::update_ethertype_and_len() {
	 // TODO: Implement
}

void cea_stream::core::build_field_offsets() {
	 // TODO: Implement
}

void cea_stream::core::filter_mutable_fields() {
	 // TODO: Implement
}

void cea_stream::core::build_payload_arrays() {
	 // TODO: Implement
}

void cea_stream::core::prepare_genspec() {
	 // TODO: Implement
}

void cea_stream::core::build_runtime() {
	 // TODO: Implement
}

void cea_stream::core::build_principal_frame() {
	 // TODO: Implement
}

void cea_stream::core::bootstrap_stream() {
	 // TODO: Implement
}

void cea_stream::core::mutate() {
	 // TODO: Implement
}

void cea_stream::core::print_stream() {
	 // TODO: Implement
}

void cea_stream::core::print_fields(vector<cea_field_mutation_spec> field_group) {
	 // TODO: Implement
}

void cea_stream::core::reset() {
	 // TODO: Implement
}

void cea_stream::core::init_stream_properties() {
	 // TODO: Implement
}

void cea_stream::core::convert_string_to_uca(string address, unsigned char *op) {
	 // TODO: Implement
}

void cea_stream::core::convert_mac_to_uca(string address, unsigned char *op) {
	 // TODO: Implement
}

void cea_stream::core::convert_ipv4_to_uca(string address, unsigned char *op) {
	 // TODO: Implement
}

void cea_stream::core::convert_ipv6_to_uca(string address, unsigned char *op) {
	 // TODO: Implement
}

uint32_t cea_stream::core::splice_frame_fields(unsigned char *buf) {
	 return 0; // TODO: Implement
}

unsigned char cea_stream::core::convert_char_to_int(string hexNumber) {
	 return ' '; // TODO: Implement
}

int cea_stream::core::convert_nibble_to_int(char digit) {
	 return 0; // TODO: Implement
}

string cea_stream::core::convert_int_to_ipv4(uint64_t ipAddress) {
	 return ""; // TODO: Implement
}

uint64_t cea_stream::core::convert_string_ipv4_internal(string addr) {
	 return 0 ; // TODO: Implement
}

void cea_stream::core::prepare_for_mutation() {
	 // TODO: Implement
}

void cea_stream::core::mutate_next_frame() {
	 // TODO: Implement
}

int cea_stream::core::mutate_enqueue(uint32_t space) {
	 return 0;// TODO: Implement
}

cea_stream::core::~core() {
    // TODO: Implement
}

// CEA STREAM BASE IMPLEMENTATION

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
