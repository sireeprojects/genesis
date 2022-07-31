#include "payload.h"
using namespace std;





int main() {

    payload *p = new payload();

    p->create_1mb_buffer();
    p->find_payload_sz();
    p->print_dimensions();

//------------------------------------------------------------------------------
// test specification (1
//------------------------------------------------------------------------------
    p->gen_type = Increment;
    cea_field_generation_spec inc_spec = {
        .value        = 0,
        .range_start  = 70,
        .range_stop   = 100,
        .range_step   = 2,
        .repeat_after = 0,
        .step         = 0
    }; 
    p->gen_spec = inc_spec;

    p->print_specification();
    p->find_size_array();

//------------------------------------------------------------------------------
// test specification (2)
//------------------------------------------------------------------------------
    p->gen_type = Random_in_Range;
    cea_field_generation_spec rnd_spec = {
        .value        = 0,
        .range_start  = 70,
        .range_stop   = 100,
        .range_step   = 0,
        .repeat_after = 0,
        .step         = 0
    }; 
    p->gen_spec = rnd_spec;

    p->print_specification();
    p->find_size_array();

//------------------------------------------------------------------------------
// test specification (3)
//------------------------------------------------------------------------------
    p->gen_type = Fixed;
    cea_field_generation_spec fxd_spec = {
        .value        = 100,
        .range_start  = 0,
        .range_stop   = 0,
        .range_step   = 0,
        .repeat_after = 0,
        .step         = 0
    }; 
    p->gen_spec = fxd_spec;

    p->print_specification();
    p->find_size_array();
    
    p->release_1mb_buffer();
    return 0;
}

