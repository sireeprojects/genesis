#include "cea_stream.h"

namespace cea {

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
    pcap *txpcap;
    pcap *rxpcap;

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


cea_stream::cea_stream(string name) {
    impl = make_unique<core>(name); 
}

cea_stream::~cea_stream() = default;

void cea_stream::set(cea_field_id id, uint64_t value) {
    impl->set(id, value);
}

void cea_stream::set(cea_field_id id, cea_field_genspec spec) {
    impl->set(id, spec);
}

void cea_stream::set(cea_stream_feature_id feature, bool mode) {
    impl->set(feature, mode);
}

void cea_stream::add_header(cea_header *header) {
    header->impl->msg_prefix = impl->msg_prefix + "|" + header->impl->msg_prefix;
    impl->frame_headers.push_back(header);
}

// TODO
void cea_stream::add_udf(cea_field *fld) {
// remember that UDF always overlays on the frame
// it does not insert new fields in the headerr or frames    
}

cea_stream::core::core(string name) {
    stream_name = name;
    stream_id = cea::stream_id;
    msg_prefix = stream_name + ":" + to_string(stream_id);
    cea::stream_id++;
    reset();
}

cea_stream::core::~core() = default;

void cea_stream::core::set(cea_field_id id, uint64_t value) {
    if (id == PAYLOAD_Pattern) {
        CEA_ERR_MSG("The field "
        << cea_trim(mtable[id].defaults.name) << " does not accepts integer values");
        abort();
    }
    // check if id is a property and then add to properties
    auto prop = find_if(stream_properties.begin(), stream_properties.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    if (prop != stream_properties.end()) {
        prop->gspec.gen_type = Fixed_Value; // TODO Check
        prop->gspec.nmr.value = value;
        prop->mdata.is_mutable = false;
    } else {
        CEA_ERR_MSG("The ID " << id << " does not belong to stream properties");
        abort();
    }
}

void cea_stream::core::set(cea_field_id id, cea_field_genspec spec) {
    // check if id is a property and then add to properties
    auto prop = find_if(stream_properties.begin(), stream_properties.end(),
        [&id](const cea_field_mutation_spec &item) {
        return (item.defaults.id == id); });

    if (prop != stream_properties.end()) {
        prop->gspec = spec;
        // TODO 
        // properties are different from mutable fields
        // check if is_mutation is applicable and needs to be set here
        // who will do the property mutation?
        //      mutate function
        //      what about runtime of the properties
        if (prop->gspec.gen_type != Fixed_Value) {
            prop->mdata.is_mutable = true;
        } else {
            prop->mdata.is_mutable = false;
        }
    } else {
        CEA_ERR_MSG("The ID " << id << " does not belong to stream properties");
        abort();
    }
}

void cea_stream::core::set(cea_stream_feature_id feature, bool mode) {
    switch (feature) {
        case PCAP_Record_Tx_Enable: {
            CEA_MSG("PCAP capture enabled @ Transmit side");
            string pcapfname = stream_name + "_tx" + + ".pcap";
            txpcap = new pcap(pcapfname, stream_name, stream_id);
            break;
            }
        case PCAP_Record_Rx_Enable: {
            CEA_MSG("PCAP capture enabled @ Receive side");
            string pcapfname = stream_name + "_rx" + + ".pcap";
            rxpcap = new pcap(pcapfname, stream_name, stream_id);
            break;
            }
        default:{
            // TODO error out with feature id not available and abort
            }
    }
}

// TODO Pending verification
uint32_t cea_stream::core::splice_frame_fields(unsigned char *buf) {
    uint32_t offset = 0;
    uint64_t mrg_data = 0;
    uint64_t mrg_len = 0;
    uint64_t mrg_cnt_total = 0;
    uint64_t mrg_cntr = 0;
    bool mrg_start = false;

    for (auto f : frame_fields) {
        if(f.defaults.merge==0) {
            if (f.defaults.type == Integer) {
                // cealog << "INT Splicing: " << f.defaults.name  << "   Offset: " << offset << endl;
                cea_memcpy_ntw_byte_order(buf+offset, (char*)&f.defaults.value, f.defaults.len/8);
            }
            else {
                // cealog << "PAT Splicing: " << f.defaults.name  << "   Offset: " << offset << endl;
                memcpy(buf+offset, f.defaults.pattern.data(), f.defaults.len/8);
                if (f.defaults.type == Pattern_IPv4) {
                    // TODO handle patterns
                }
            }
            offset += f.defaults.len/8;
        } else {
            if (!mrg_start) {
                mrg_start = true;
                mrg_cnt_total = f.defaults.merge + 1;
            }
            mrg_data = (mrg_data << f.defaults.len) | f.defaults.value;
            mrg_len = mrg_len + f.defaults.len;
            mrg_cntr++;

            if (mrg_cntr == mrg_cnt_total) {
                cea_memcpy_ntw_byte_order(buf+offset, (char*)&mrg_data, mrg_len/8);
                offset += mrg_len/8;
                mrg_data = 0; 
                mrg_len = 0;
                mrg_cntr = 0;
                mrg_cnt_total = 0;
                mrg_start = false;
            }
        }
    }
    return offset;
}
 
// TODO handle PROPERTIES mutation and runtime 
void cea_stream::core::bootstrap_stream() {
    collate_frame_fields();
    update_ethertype_and_len();
    build_field_offsets();
    filter_mutable_fields();
    prepare_genspec();
    build_runtime();
    // print_stream();
    build_payload_arrays();
    build_principal_frame();
}

// TODO display warning (why?) when frame headers are empty
void cea_stream::core::collate_frame_fields() {
    frame_fields.clear();
    for (auto f : frame_headers) {
        frame_fields.insert(
            frame_fields.end(),
            f->impl->header_fields.begin(),
            f->impl->header_fields.end()
        );
    }
}
 
// TODO pending implementation
void cea_stream::core::update_ethertype_and_len() {
}

// TODO display warning (why?) when frame headers are empty
void cea_stream::core::build_field_offsets() {
    vector<cea_field_mutation_spec>::iterator it;
    
    it = frame_fields.begin();
    it->mdata.offset = 0;
    hdr_len = hdr_len + it->defaults.len;

    // for(it=frame_fields.begin(); it<frame_fields.end(); it++) {
    for(it=frame_fields.begin()+1; it<frame_fields.end(); it++) {
        it->mdata.offset = prev(it)->defaults.len + prev(it)->mdata.offset;
        // cealog << "Field: " << it->defaults.name << "  Offset: " << dec << it->mdata.offset << endl;
        hdr_len = hdr_len + it->defaults.len;
    }
    for(it=frame_fields.begin(); it<frame_fields.end(); it++) {
        // cealog << "Field: " << it->defaults.name << "  Offset: " << dec << it->mdata.offset << endl;
        // it->mdata.offset = prev(it)->defaults.len + prev(it)->mdata.offset;
        // hdr_len = hdr_len + it->defaults.len;
    }
}

// TODO display info about the mutable fields
void cea_stream::core::filter_mutable_fields() {
    mutable_fields.clear();
    for (auto f : frame_fields) {
        if (f.mdata.is_mutable) {
            mutable_fields.push_back(f);
        }
    }
}
 
void cea_stream::core::print_fields(vector<cea_field_mutation_spec> field_group) {
    stringstream ss;
    ss.setf(ios_base::left);

    for(auto item : field_group) {
        ss << setw(5)  << left << item.defaults.id ;
        ss << setw(25) << left << item.defaults.name;
        ss << setw(25) << left << item.defaults.len;
        ss << setw(25) << left << cea_field_type_name[item.defaults.type];;
        if (item.defaults.type == Integer)
            ss << setw(25) << left << item.gspec.nmr.value;
        else
            ss << setw(25) << left << item.gspec.str.value;
        ss << setw(25) << left << cea_gen_type_name[item.gspec.gen_type];
        ss << endl;
    }
    ss << endl;
    cealog << ss.str();
}

// TODO display in a better format with sub headings
void cea_stream::core::print_stream() {
    for (auto f : frame_headers) {
        cealog << cea_header_name[f->impl->header_type] << endl;
        for (auto item : f->impl->header_fields) {
            cealog << "  |--" << item.defaults.name << endl;
        }
    }
    print_fields(stream_properties);
    print_fields(mutable_fields);
}

void cea_stream::core::build_payload_arrays() {
// TODO Support API to quickly set payload without a spec
// void set(cea_field_id id, enum of PREDFINED_PAYLOAD_PATTERN);
// void set(cea_field_id id, string pattern, bool fill=true);

    //-------------
    // size arrays
    //-------------
    // auto len_item = get_field(stream_properties, FRAME_Len);
    // cea_field_genspec spec = len_item.gspec;

    // TODO check if this works
    cea_field_genspec spec = (get_field(stream_properties, FRAME_Len)).gspec;
    cea_field_random rnd = (get_field(stream_properties, FRAME_Len)).rnd;

    nof_sizes = 0;

    switch (spec.gen_type) {
        case Fixed_Value: {
            nof_sizes = 1;
            vof_frame_sizes.resize(nof_sizes);
            vof_computed_frame_sizes.resize(nof_sizes);
            vof_payload_sizes.resize(nof_sizes);
            vof_frame_sizes[0] = spec.nmr.value;
            vof_computed_frame_sizes[0] = vof_frame_sizes[0] + meta_size;
            vof_payload_sizes[0] = vof_frame_sizes[0] - (hdr_size - meta_size) - crc_len;
            break;
            }
        case Increment: {
            nof_sizes = ((spec.nmr.max - spec.nmr.min)/spec.nmr.step)+1;
            vof_frame_sizes.resize(nof_sizes);
            vof_computed_frame_sizes.resize(nof_sizes);
            vof_payload_sizes.resize(nof_sizes);
            uint32_t szidx=0;
            for (uint32_t i=spec.nmr.min; i<=spec.nmr.max; i=i+spec.nmr.step) {
                vof_frame_sizes[szidx] = i;
                vof_computed_frame_sizes[szidx] = vof_frame_sizes[szidx] + meta_size;
                vof_payload_sizes[szidx] = vof_frame_sizes[szidx] - (hdr_size - meta_size) - crc_len;
                szidx++;
            }
            break;
            }
        case Decrement: {
            nof_sizes = ((spec.nmr.min - spec.nmr.max)/spec.nmr.step)+1;
            vof_frame_sizes.resize(nof_sizes);
            vof_computed_frame_sizes.resize(nof_sizes);
            vof_payload_sizes.resize(nof_sizes);
            uint32_t szidx=0;
            for (uint32_t i=spec.nmr.min; i>=spec.nmr.max; i=i-spec.nmr.step) {
                vof_frame_sizes[szidx] = i;
                vof_computed_frame_sizes[szidx] = vof_frame_sizes[szidx] + meta_size;
                vof_payload_sizes[szidx] = vof_frame_sizes[szidx] - (hdr_size - meta_size) - crc_len;
                szidx++;
            }
            break;
            }
        case Random: {
            nof_sizes = (spec.nmr.max - spec.nmr.min) + 1;
            vof_frame_sizes.resize(nof_sizes);
            vof_computed_frame_sizes.resize(nof_sizes);
            vof_payload_sizes.resize(nof_sizes);
            // random_device rd;
            // mt19937 gen(rd());
            // uniform_int_distribution<> distr(spec.nmr.max, spec.nmr.min);
            // vof_frame_sizes[szidx] = distr(rnd.engine);
            //
            // TODO check if the seed selection shud be done here on is it 
            // already taken care in build_runtime
            // if (spec.nmr.seed != 0) {
            //     rnd.engine.seed(spec.nmr.seed);
            // } else {
            //     rnd.engine.seed(rd());
            // }
            // uint32_t szidx=0;
            for (uint32_t szidx=spec.nmr.min; szidx>spec.nmr.max; szidx++) {
                vof_frame_sizes[szidx] = rnd.ud(rnd.engine);
                vof_computed_frame_sizes[szidx] = vof_frame_sizes[szidx] + meta_size;
                vof_payload_sizes[szidx] = vof_frame_sizes[szidx] - (hdr_size - meta_size) - crc_len;
            }
            break;
            }
        case Weighted_Distribution: { // TODO
            nof_sizes = CEA_WEIGHTED_DISTR_RECYCLE_LIMIT;
            vof_frame_sizes.resize(nof_sizes);
            vof_computed_frame_sizes.resize(nof_sizes);
            vof_payload_sizes.resize(nof_sizes);
            // uint32_t szidx=0;
            for (uint32_t szidx=0; szidx>nof_sizes; szidx++) {
                vof_frame_sizes[szidx] = rnd.wd(rnd.engine);
                vof_computed_frame_sizes[szidx] = vof_frame_sizes[szidx] + meta_size;
                vof_payload_sizes[szidx] = vof_frame_sizes[szidx] - (hdr_size - meta_size) - crc_len;
            }
            break;
            }
        default:{
            CEA_MSG("Invalid Generation type Specified for Frame Length");
            exit(1);
            }
    }
    
    //---------------
    // payload array
    //---------------
    arof_payload_data = new unsigned char[CEA_MAX_FRAME_SIZE];
    // auto pl_item = get_field(stream_properties, PAYLOAD_Pattern);
    // cea_field_genspec plspec = pl_item.gspec;

    // TODO check if this works
    cea_field_genspec plspec = (get_field(stream_properties, PAYLOAD_Pattern)).gspec;

    switch (plspec.gen_type) {
        case Random : {
            // create random arrays and fill it with random data
            srand(time(NULL));
            for (uint32_t idx=0; idx<CEA_MAX_RND_ARRAYS; idx++) {
                uint32_t array_size = CEA_MAX_FRAME_SIZE + CEA_RND_ARRAY_SIZE;
                arof_rnd_payload_data[idx] = new unsigned char[array_size];
                for(uint32_t offset=0; offset<array_size; offset++) {
                    int num = rand()%255;
                    memcpy(arof_rnd_payload_data[idx]+offset, (unsigned char*)&num, 1);
                }
            }
            break;
            }
        case Fixed_Value: {
            payload_pattern_size = plspec.str.value.size() / 2;
            payload_pattern = new unsigned char[payload_pattern_size];
            convert_string_to_uca(plspec.str.value, payload_pattern);

            uint32_t quotient = CEA_MAX_FRAME_SIZE/payload_pattern_size; 
            uint32_t remainder = CEA_MAX_FRAME_SIZE%payload_pattern_size;
            uint32_t offset = 0;

            if (plspec.str.repeat) {
                for (uint32_t cnt=0; cnt<quotient; cnt++) {
                    memcpy(arof_payload_data+offset, payload_pattern, payload_pattern_size);
                    offset += payload_pattern_size;
                }
                memcpy(arof_payload_data+offset, payload_pattern, remainder);
            } else {
                memcpy(arof_payload_data+offset, payload_pattern, payload_pattern_size);
            }
            delete [] payload_pattern;
            break;
            }
        case Increment_Byte: {
            uint32_t offset = 0;
            for (uint32_t idx=0; idx<CEA_MAX_FRAME_SIZE/256; idx++) {
                for (uint16_t val=0; val<256; val++) {
                    memcpy(arof_payload_data+offset, (char*)&val, 1);
                    offset++;
                }
            }
            break;
            }
        case Increment_Word: {
            uint32_t offset = 0;
            for (uint32_t idx=0; idx<CEA_MAX_FRAME_SIZE/2; idx++) {
                cea_memcpy_ntw_byte_order(arof_payload_data+offset, (char*)&idx, 2);
                offset += 2;
            }
            break;
            }
        case Decrement_Byte: {
            uint32_t offset = 0;
            for (uint32_t idx=0; idx<CEA_MAX_FRAME_SIZE/256; idx++) {
                for (int16_t val=255; val>=0; val--) {
                    memcpy(arof_payload_data+offset, (char*)&val, 1);
                    offset++;
                }
            }
            break;
            }
        case Decrement_Word: {
            uint32_t offset = 0;
            for (uint32_t val=0xFFFF; val>=0; val--) {
                memcpy(arof_payload_data+offset, (char*)&val, 2);
                offset += 2;
                if (offset > CEA_MAX_FRAME_SIZE) break;
            }
            break;
            }
        default:{
            CEA_MSG("Invalid Generation type Specified for Frame payload: " << cea_gen_type_name[plspec.gen_type]);
            exit(1);
            // TODO exit or abort
            }
    }
}

void cea_stream::core::prepare_genspec() {
    for (auto &m : mutable_fields) {
        switch (m.defaults.type) {
            case Pattern_MAC: {
                m.gspec.nmr.step = m.gspec.str.step;
                m.gspec.nmr.count = m.gspec.str.count;
                m.gspec.nmr.repeat = m.gspec.str.repeat;
                m.gspec.nmr.seed = m.gspec.str.seed;
                m.gspec.nmr.error = m.gspec.str.error;

                uint64_t tmp_mac;
                string tmp_mac_string;

                if (m.gspec.str.value.size() > 0) {
                    tmp_mac_string = m.gspec.str.value;
                    tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                    tmp_mac = stoul(tmp_mac_string, 0, 16);
                    m.gspec.nmr.value = tmp_mac;
                }

                if (m.gspec.str.min.size() > 0) {
                    tmp_mac_string = m.gspec.str.min;
                    tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                    tmp_mac = stol(tmp_mac_string, 0, 16);
                    m.gspec.nmr.min = tmp_mac;
                }

                if (m.gspec.str.max.size() > 0) {
                    tmp_mac_string = m.gspec.str.max;
                    tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                    tmp_mac = stol(tmp_mac_string, 0, 16);
                    m.gspec.nmr.max = tmp_mac;
                }

                if (m.gspec.str.mask.size() > 0) {
                    tmp_mac_string = m.gspec.str.mask;
                    tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                    tmp_mac = stol(tmp_mac_string, 0, 16);
                    m.gspec.nmr.mask = tmp_mac;
                }

                if (m.gspec.str.start.size() > 0) {
                    tmp_mac_string = m.gspec.str.start;
                    tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                    tmp_mac = stol(tmp_mac_string, 0, 16);
                    m.gspec.nmr.start = tmp_mac;
                }

                for (uint32_t idx=0; idx<m.gspec.str.values.size(); idx++) {
                    if (m.gspec.str.values[idx].size() > 0) {
                        tmp_mac_string = m.gspec.str.values[idx];
                        tmp_mac_string.erase(remove(tmp_mac_string.begin(), tmp_mac_string.end(), ':'), tmp_mac_string.end());
                        tmp_mac = stol(tmp_mac_string, 0, 16);
                        m.gspec.nmr.values.push_back(tmp_mac);
                    }
                }
                break;
                }
            case Pattern_IPv4: {
                m.gspec.nmr.step = m.gspec.str.step;
                m.gspec.nmr.count = m.gspec.str.count;
                m.gspec.nmr.repeat = m.gspec.str.repeat;
                m.gspec.nmr.seed = m.gspec.str.seed;
                m.gspec.nmr.error = m.gspec.str.error;

                string tmp_ipv4_string;
                uint64_t tmp_ipv4;

                if (m.gspec.str.value.size() > 0) {
                    tmp_ipv4_string = m.gspec.str.value;
                    tmp_ipv4 = convert_string_ipv4_internal(tmp_ipv4_string);
                    m.gspec.nmr.value = tmp_ipv4;
                }

                if (m.gspec.str.min.size() > 0) {
                    tmp_ipv4_string = m.gspec.str.min;
                    tmp_ipv4 = convert_string_ipv4_internal(tmp_ipv4_string);
                    m.gspec.nmr.min = tmp_ipv4;
                }

                if (m.gspec.str.max.size() > 0) {
                    tmp_ipv4_string = m.gspec.str.max;
                    tmp_ipv4 = convert_string_ipv4_internal(tmp_ipv4_string);
                    m.gspec.nmr.max = tmp_ipv4;
                }

                if (m.gspec.str.mask.size() > 0) {
                    tmp_ipv4_string = m.gspec.str.mask;
                    tmp_ipv4 = convert_string_ipv4_internal(tmp_ipv4_string);
                    m.gspec.nmr.mask = tmp_ipv4;
                }

                if (m.gspec.str.start.size() > 0) {
                    tmp_ipv4_string = m.gspec.str.start;
                    tmp_ipv4 = convert_string_ipv4_internal(tmp_ipv4_string);
                    m.gspec.nmr.start = tmp_ipv4;
                }

                for (uint32_t idx=0; idx<m.gspec.str.values.size(); idx++) {
                    if (m.gspec.str.values[idx].size() > 0) {
                        tmp_ipv4_string = m.gspec.str.values[idx];
                        tmp_ipv4 = convert_string_ipv4_internal(tmp_ipv4_string);
                        m.gspec.nmr.values.push_back(tmp_ipv4);
                    }
                }
                break;
                }
            case Pattern_PRE: {
                // Only Fixed size is allowed
                m.gspec.nmr.seed = m.gspec.str.seed;
                m.gspec.nmr.error = m.gspec.str.error;
                if (m.gspec.str.value.size() > 0) {
                    m.gspec.nmr.value = stoul(m.gspec.str.value, 0, 16);
                }
                break;
                }
            default: {}
        }
    }
}

//TODO preamble and ipv6 support is pending
void cea_stream::core::build_runtime() {
    for (auto &m : mutable_fields) {
        switch (m.gspec.gen_type) {
            case Fixed_Value: {
                m.rt.value = m.gspec.nmr.value;
                break;
                }
            case Increment: {
                m.rt.value = m.gspec.nmr.start;
                break;
                }
            case Decrement: {
                m.rt.value = m.gspec.nmr.start;
                break;
                }
            case Value_List: {
                m.rt.patterns = m.gspec.nmr.values;
                break;
                }
            case Random: {
                // check and set seed
                if (m.gspec.nmr.seed != 0) {
                    m.rnd.engine.seed(m.gspec.nmr.seed);
                } else {
                    m.rnd.engine.seed(rd());
                }

                uint64_t size = m.defaults.len/8;

                if (is_number_in_range(size, 1, 8)) {
                    uniform_int_distribution<uint64_t>::param_type 
                        u8param(0, numeric_limits<unsigned char>::max());
                    m.rnd.ud.param(u8param);
                } else if (is_number_in_range(size, 9, 16)) {
                    uniform_int_distribution<uint64_t>::param_type 
                        u16param(0, numeric_limits<unsigned short>::max());
                    m.rnd.ud.param(u16param);
                } else if (is_number_in_range(size, 17, 32)) {
                    uniform_int_distribution<uint64_t>::param_type 
                        u32param(0, numeric_limits<uint32_t>::max());
                    m.rnd.ud.param(u32param);
                } else if (is_number_in_range(size, 33, 64)) {
                    uniform_int_distribution<uint64_t>::param_type 
                        u64param(0, numeric_limits<uint64_t>::max());
                    m.rnd.ud.param(u64param);
                }
                m.rt.value = m.rnd.ud(m.rnd.engine);
                break;
                }
            case Random_In_Range: {
                // check and set seed
                if (m.gspec.nmr.seed != 0) {
                    m.rnd.engine.seed(m.gspec.nmr.seed);
                } else {
                    m.rnd.engine.seed(rd());
                }
                uniform_int_distribution<uint64_t>::param_type 
                    param(m.gspec.nmr.min, m.gspec.nmr.max);
                m.rnd.ud.param(param);
                m.rt.value = m.rnd.ud(m.rnd.engine);
                break;
                }
            case Weighted_Distribution: { // TODO only for frame size mutation
                if (m.gspec.nmr.seed != 0) {
                    m.rnd.engine.seed(m.gspec.nmr.seed);
                } else {
                    m.rnd.engine.seed(rd());
                }
                for (auto item : m.gspec.nmr.distr) {
                    m.rnd.wd_lenghts.push_back(item.first);
                }
                for (auto item : m.gspec.nmr.distr) {
                    m.rnd.wd_weights.push_back(item.second);
                }
                discrete_distribution<uint64_t>::param_type
                    param(m.rnd.wd_weights.begin(), m.rnd.wd_weights.end());
                m.rnd.wd.param(param);
                // generate lenght values in build_payload_arrays
                break;
                }
            default: {
                // TODO add message
                }
        } // switch
    } // for
}

// TODO Incomplete implementation
void cea_stream::core::build_principal_frame() {

    // print_fields(frame_fields);
    uint32_t ofs = splice_frame_fields(pf);

    auto len_item = get_field(stream_properties, FRAME_Len);
    cea_field_genspec lenspec = len_item.gspec;

    auto pl_item = get_field(stream_properties, PAYLOAD_Pattern);
    cea_field_genspec plspec = pl_item.gspec;

    uint32_t ploffset = hdr_len/8;

    if (plspec.gen_type == Random)
        memcpy(pf+ploffset, arof_rnd_payload_data[0], lenspec.nmr.value);
    else 
        memcpy(pf+ploffset, arof_payload_data, lenspec.nmr.value);

    print_uchar_array(pf, ploffset+lenspec.nmr.value, "Principal Frame");
    // txpcap->write(pf, ploffset+lenspec.nmr.value); 
}


// TODO enclose mutate with perf timers
// TODO what if there are no mutables
void cea_stream::core::mutate() {
    auto burst_spec = (get_field(stream_properties, STREAM_Burst_Size)).gspec;
    uint32_t num_txn = burst_spec.nmr.value;

    vector<cea_field_mutation_spec> mut = mutable_fields;

    auto len_item = get_field(stream_properties, FRAME_Len);
    cea_field_genspec lenspec = len_item.gspec;

    cea_timer stopwatch;
    stopwatch.start();

    for (uint64_t nof_frames=0; nof_frames<num_txn; nof_frames++) {
        for (auto m=begin(mut); m!=end(mut); m++) {
            switch(m->defaults.type) {
                case Integer: {
                    switch(m->gspec.gen_type) {
                        case Fixed_Value: { // TESTED
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->gspec.nmr.value, m->defaults.len/8);
                            m->mdata.is_mutable = false;
                            break;
                            }
                        case Value_List: { // TESTED
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.patterns[m->rt.idx], m->defaults.len/8);
                            if (m->rt.idx < m->rt.patterns.size()-1) {
                                m->rt.idx++;
                            } else {
                                if (m->gspec.nmr.repeat) {
                                    m->rt.idx = 0;
                                } else {
                                    m->mdata.is_mutable = false;
                                }
                            }
                            break;
                            }
                        case Increment: { // TESTED
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                            if (m->rt.count < m->gspec.nmr.count-1) {
                                m->rt.value += m->gspec.nmr.step;
                                m->rt.count++;
                            } else {
                                if (m->gspec.nmr.repeat) {
                                    m->rt.count = 0;
                                    m->rt.value = m->gspec.nmr.start;
                                } else {
                                    m->mdata.is_mutable = false;
                                }
                            }
                            break;
                            }
                        case Decrement: { // TESTED
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                            if (m->rt.count < m->gspec.nmr.count-1) {
                                m->rt.value -= m->gspec.nmr.step;
                                m->rt.count++;
                            } else {
                                if (m->gspec.nmr.repeat) {
                                    m->rt.count = 0;
                                    m->rt.value = m->gspec.nmr.start;
                                } else {
                                    m->mdata.is_mutable = false;
                                }
                            }
                            break;
                            }
                        case Random: { // TODO testing
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                            m->rt.value = m->rnd.ud(m->rnd.engine);
                            break;
                            }
                        case Random_In_Range: { // TODO testing
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                            m->rt.value = m->rnd.ud(m->rnd.engine);
                            break;
                            }
                        default: {}
                    }
                    break;
                    } // Integer
                // TODO add support for preamble and ipv6
                case Pattern_MAC:
                case Pattern_IPv4: {
                    switch(m->gspec.gen_type) {
                        case Fixed_Value: {
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                            m->mdata.is_mutable = false;
                            break;
                            }
                        case Value_List: {
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.patterns[m->rt.idx], m->defaults.len/8);
                            if (m->rt.idx < m->rt.patterns.size()-1) {
                                m->rt.idx++;
                            } else {
                                if (m->gspec.nmr.repeat) {
                                    m->rt.idx = 0;
                                } else {
                                    m->mdata.is_mutable = false;
                                }
                            }
                            break;
                            }
                        case Increment: {
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                            if (m->rt.count < m->gspec.nmr.count-1) {
                                m->rt.value += m->gspec.nmr.step;
                                m->rt.count++;
                            } else {
                                if (m->gspec.nmr.repeat) {
                                    m->rt.count = 0;
                                    m->rt.value = m->gspec.nmr.start;
                                }
                            }
                            break;
                            }
                        case Decrement: {
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                            if (m->rt.count < m->gspec.nmr.count-1) {
                                m->rt.value -= m->gspec.nmr.step;
                                m->rt.count++;
                            } else {
                                if (m->gspec.nmr.repeat) {
                                    m->rt.count = 0;
                                    m->rt.value = m->gspec.nmr.start;
                                }
                            }
                            break;
                            }
                        case Random: {
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                            m->rt.value = m->rnd.ud(m->rnd.engine);
                            break;
                            }
                        case Random_In_Range: {
                            cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                            m->rt.value = m->rnd.ud(m->rnd.engine);
                            break;
                            }
                        default: {}
                    }
                    break;
                    }
                default: {}
            }
        } // mutation loop
        // auto len_item = get_field(stream_properties, FRAME_Len);
        // cea_field_genspec lenspec = len_item.gspec;

        uint32_t ploffset = hdr_len/8;
        // print_uchar_array_1n(pf, ploffset+14, "Mutated Frame");
        // txpcap->write(pf, ploffset+lenspec.nmr.value); 
        // print_uchar_array_1n(pf, ploffset+32, "Mutated Frame");
        // print_uchar_array(pf, ploffset+lenspec.nmr.value, "Mutated Frame");
       
        // TODO copy frame to transmit buffer
        memcpy(test_buffer, pf, 64);
        // memcpy(test_buffer, pf, ploffset+lenspec.nmr.value);

        for (auto m=begin(mut); m!=end(mut);) {
            if (m->mdata.is_mutable == false) {
                mut.erase(m);
            } else {
                m++;
            }
        }
    } // num_txn
    cealog << "Time taken: " << stopwatch.elapsed_in_string() << endl;

}

void cea_stream::core::init_stream_properties() {
    stream_properties.clear();
    vector<cea_field_id> prop_ids =  header_to_field_map[PROPERTIES];

    for (auto id : prop_ids) {
        auto item = get_field(mtable, id);
        stream_properties.push_back(item);
    }
}
 
void cea_stream::core::reset() {
    frame_headers.clear();

    // add metadata to headers by default
    // cea_header *meta = new cea_header(META);
    // frame_headers.push_back(meta);

    frame_fields.clear();
    udfs.clear();
    init_stream_properties();

    // TODO memory leak when reset is done twice in same test
    pf = new unsigned char [CEA_PF_SIZE];

    payload_pattern_size = 0;

    // TODO why does the following crash
    // if (payload_pattern != nullptr) {
    //     delete [] payload_pattern;
    //     payload_pattern = nullptr;
    // }

    hdr_len = 0;
}

void cea_stream::core::convert_string_to_uca(string address, unsigned char *op) {
    vector <string> tokens;

    for (uint32_t i=0; i<address.size(); i+=2) {
        tokens.push_back(address.substr(i, 2));
    }
    for (uint32_t i=0; i<address.size()/2; i++) {
        op[i]= convert_char_to_int(tokens[i]);
    }
}

void cea_stream::core::convert_mac_to_uca(string address, unsigned char *op) {
    stringstream check1(address);
    vector <string> tokens;
    string intermediate;

    while(getline(check1, intermediate, ':')) {
        tokens.push_back(intermediate);
    }
    for (uint32_t i=0; i<6; i++) {
        op[i]= convert_char_to_int(tokens[i]);
    }
}

void cea_stream::core::convert_ipv4_to_uca(string address, unsigned char *op) {
    stringstream check1(address);
    string intermediate;
    int i = 0;

    while(getline(check1, intermediate, '.')) {
        op[i] = stoi(intermediate);
        i++;
    }
}

void cea_stream::core::convert_ipv6_to_uca(string address, unsigned char *op) {
    string ipv6addr_tmp = address;
    vector <string> tokens;

    ipv6addr_tmp.erase(remove(ipv6addr_tmp.begin(), ipv6addr_tmp.end(), ':'),
        ipv6addr_tmp.end());

    for (size_t i=0; i<ipv6addr_tmp.size(); i+=2) {
        tokens.push_back(ipv6addr_tmp.substr(i, 2));
    }
    for (uint32_t i=0; i<16; i++) { // TODO remove hardcoded value
        op[i]= convert_char_to_int(tokens[i]);
    }
}

string cea_stream::core::convert_int_to_ipv4(uint64_t ipAddress) {
    uint32_t octet1 = (ipAddress >> 24) & 0xFF;
    uint32_t octet2 = (ipAddress >> 16) & 0xFF;
    uint32_t octet3 = (ipAddress >> 8) & 0xFF;
    uint32_t octet4 = ipAddress & 0xFF;
    return (to_string(octet1) + "." +
        to_string(octet2) + "." +
        to_string(octet3) + "." +
        to_string(octet4));
}

uint64_t cea_stream::core::convert_string_ipv4_internal(string addr) {
    stringstream s;
    string intermediate;
    stringstream check1(addr);
    int i = 0;
    while(getline(check1, intermediate, '.')) {
        s << setfill('0') << setw(2) << hex << stoi(intermediate);
        i++;
    }
    return stoul(s.str(), 0, 16);
}

int cea_stream::core::convert_nibble_to_int (char digit) {
    int asciiOffset, digitValue;
    if (digit >= 48 && digit <= 57) {
        // code for '0' through '9'
        asciiOffset = 48;
        digitValue = digit - asciiOffset;
        return digitValue;
    } else if (digit >= 65 && digit <= 70) {
        // digit is 'A' through 'F'
        asciiOffset = 55;
        digitValue = digit - asciiOffset;
        return digitValue;
    } else if (digit >= 97 && digit <= 122) {
        // code for 'a' through 'f'
        asciiOffset = 87;
        digitValue = digit - asciiOffset;
        return digitValue;
    } else {
        // TODO illegal digit
    }
    return 0;
}

unsigned char cea_stream::core::convert_char_to_int(string hexNumber) {
     unsigned char aChar;
     char highOrderDig = hexNumber[0];
     char lowOrderDig  = hexNumber[1];
     int lowOrderValue = convert_nibble_to_int(lowOrderDig);
     //  convert lowOrderDig to number from 0 to 15
     int highOrderValue = convert_nibble_to_int(highOrderDig);
     // convert highOrderDig to number from 0 to 15
     aChar = lowOrderValue + 16 * highOrderValue;
     return aChar;
}

void cea_stream::core::prepare_for_mutation() {
    num_txns = ((get_field(stream_properties, STREAM_Burst_Size)).gspec).nmr.value;
    mut = mutable_fields;
    lenspec = (get_field(stream_properties, FRAME_Len)).gspec;

    num_txns_transmitted = 0;
    offset = 0;
    txdone = true;
    num_elems = 0;
    num_elems_transmitted = 0;
    stream_done = false;
    state = NEW_FRAME;
}

int cea_stream::core::mutate_enqueue(uint32_t space) {
    if (stream_done) return 1;

    uint32_t space_avail = space;

    while (space_avail > 0) {
        cealog << "State: " << ((state==0) ? "NEW FRAME" : "TRANSMIT")  << endl;
        switch (state) {
            case NEW_FRAME:
                if (txdone) {
                    mutate_next_frame();
                    cealog << "Frame Size: " << lenspec.nmr.value << endl;
                    num_elems = ceil( (double)(lenspec.nmr.value)/64); // IFWIDTH=64B
                    cealog << "Num Elems: " << num_elems << endl;
                    state = TRANSMIT;
                    num_elems_transmitted = 0;
                    offset = 0;
                    txdone = false;
                } else {
                    state = TRANSMIT;
                }
                break;

            case TRANSMIT:
                DataQ_put((unsigned int*)pf+offset); // TODO Check
                num_elems_transmitted++;
                offset += 64;
                space_avail--;
                if (num_elems_transmitted == num_elems) {
                    cealog << "Transmitting: " << num_elems_transmitted << endl;
                    txdone = true;
                    num_txns_transmitted++;
                    if (num_txns_transmitted == num_txns) {
                        stream_done = true;
                        return 1;
                    }
                    state = NEW_FRAME;
                }
                break;
            } //switch
    }
    return 0;
}

void cea_stream::core::mutate_next_frame() {
    for (auto m=begin(mut); m!=end(mut); m++) {
        switch(m->defaults.type) {
            case Integer: {
                switch(m->gspec.gen_type) {
                    case Fixed_Value: { // TESTED
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->gspec.nmr.value, m->defaults.len/8);
                        m->mdata.is_mutable = false;
                        break;
                        }
                    case Value_List: { // TESTED
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.patterns[m->rt.idx], m->defaults.len/8);
                        if (m->rt.idx < m->rt.patterns.size()-1) {
                            m->rt.idx++;
                        } else {
                            if (m->gspec.nmr.repeat) {
                                m->rt.idx = 0;
                            } else {
                                m->mdata.is_mutable = false;
                            }
                        }
                        break;
                        }
                    case Increment: { // TESTED
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                        if (m->rt.count < m->gspec.nmr.count-1) {
                            m->rt.value += m->gspec.nmr.step;
                            m->rt.count++;
                        } else {
                            if (m->gspec.nmr.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->gspec.nmr.start;
                            } else {
                                m->mdata.is_mutable = false;
                            }
                        }
                        break;
                        }
                    case Decrement: { // TESTED
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                        if (m->rt.count < m->gspec.nmr.count-1) {
                            m->rt.value -= m->gspec.nmr.step;
                            m->rt.count++;
                        } else {
                            if (m->gspec.nmr.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->gspec.nmr.start;
                            } else {
                                m->mdata.is_mutable = false;
                            }
                        }
                        break;
                        }
                    case Random: { // TODO testing
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                        m->rt.value = m->rnd.ud(m->rnd.engine);
                        break;
                        }
                    case Random_In_Range: { // TODO testing
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                        m->rt.value = m->rnd.ud(m->rnd.engine);
                        break;
                        }
                    default: {}
                }
                break;
                } // Integer
            // TODO add support for preamble and ipv6
            case Pattern_MAC:
            case Pattern_IPv4: {
                switch(m->gspec.gen_type) {
                    case Fixed_Value: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                        m->mdata.is_mutable = false;
                        break;
                        }
                    case Value_List: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.patterns[m->rt.idx], m->defaults.len/8);
                        if (m->rt.idx < m->rt.patterns.size()-1) {
                            m->rt.idx++;
                        } else {
                            if (m->gspec.nmr.repeat) {
                                m->rt.idx = 0;
                            } else {
                                m->mdata.is_mutable = false;
                            }
                        }
                        break;
                        }
                    case Increment: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                        if (m->rt.count < m->gspec.nmr.count-1) {
                            m->rt.value += m->gspec.nmr.step;
                            m->rt.count++;
                        } else {
                            if (m->gspec.nmr.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->gspec.nmr.start;
                            }
                        }
                        break;
                        }
                    case Decrement: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                        if (m->rt.count < m->gspec.nmr.count-1) {
                            m->rt.value -= m->gspec.nmr.step;
                            m->rt.count++;
                        } else {
                            if (m->gspec.nmr.repeat) {
                                m->rt.count = 0;
                                m->rt.value = m->gspec.nmr.start;
                            }
                        }
                        break;
                        }
                    case Random: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                        m->rt.value = m->rnd.ud(m->rnd.engine);
                        break;
                        }
                    case Random_In_Range: {
                        cea_memcpy_ntw_byte_order(pf+m->mdata.offset/8, (char*)&m->rt.value, m->defaults.len/8);
                        m->rt.value = m->rnd.ud(m->rnd.engine);
                        break;
                        }
                    default: {}
                }
                break;
                }
            default: {}
        }
    } // mutation loop
}

}
