#include "cea_common.h"

namespace cea {

class cea_stream {
public:    
    cea_stream(string name = "stream");
    ~cea_stream();
    void set(cea_field_id id, uint64_t value); // to set property only
    void set(cea_field_id id, cea_field_genspec spec); // to set property only
    void set(cea_stream_feature_id feature, bool mode);
    void add_header(cea_header *header);
    void add_udf(cea_field *field);
    // TODO Support AVIP type test case
    // TODO Support AVIP type custom payload
    //
    // TODO Support API to quickly set payload without a spec
    // void set(cea_field_id id, enum of PREDFINED_PAYLOAD_PATTERN);
    // void set(cea_field_id id, string pattern, bool fill=true);
private:
    class core;
    unique_ptr<core> impl;
    friend class cea_port;
    friend class cea_controller;
};

}
