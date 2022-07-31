#include "payload.h"
using namespace std;

#define ONE_MB (1024*1024)
unsigned char *buf;
uint32_t hdr_size = 14;
uint32_t fcs_size = 4;
uint32_t frm_size = 64;
uint32_t pl_size = 0;

cea_field_id gen_field;
cea_field_generation_type gen_type;
cea_field_generation_spec gen_spec;

uint32_t *size_idx;
uint32_t *start_idx;


int main() {
    // create the 1 MByte frame buffer
    buf = new unsigned char[ONE_MB];

    // find payload size
    pl_size = frm_size - (hdr_size + fcs_size);

    cout << endl << cea_formatted_hdr("Frame Dimensions");
    cealog << toc(30, "Configured Frame Size") << frm_size << endl;
    cealog << toc(30, "Header size") << hdr_size << endl;
    cealog << toc(30, "FCS size") << fcs_size << endl;
    cealog << toc(30, "Payload size") << pl_size << endl;

    // test specification
    gen_type = Random_in_Range;
    gen_spec = {0, 70, 100, 0, 0, 0};

    // display the test specification
    cealog << endl << cea_formatted_hdr("Payload Specification");
    cealog << toc(30, "Payload Type") << to_str(gen_type) << endl;
    cealog << to_str(gen_spec) << endl;
    
    // process size and generate size_idx
    switch (gen_type) {
        case Fixed: {
             size_idx = new uint32_t;
             // store only single value
             *size_idx = frm_size;
             break;
             }
        case Random_in_Range: {
             // determine the number of sizes allowed
             // and allocate memeory
             uint32_t nof_sizes = gen_spec.range_stop - gen_spec.range_start;
             size_idx = new uint32_t[nof_sizes];
            
             // generate and store random sizes from the given range
             randomize_uint_buf(size_idx, gen_spec.range_start, gen_spec.range_stop, nof_sizes);
             print_uint(size_idx, nof_sizes);
             break;
             }
        case Increment: {
             // determine the number of sizes allowed
             // and allocate memeory
             uint32_t nof_sizes = (gen_spec.range_stop - gen_spec.range_start) / gen_spec.range_step;
             size_idx = new uint32_t[nof_sizes];
             break;
             }
        default:{
            cealog << "***ERROR: Illegal gen specification" << endl;
            abort();
        }
    }
    
    // release ONE_MB buffer
    delete(buf);
    return 0;
}

