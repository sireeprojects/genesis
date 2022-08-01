#include "payload.h"
using namespace std;

int main() {
    payload *p = new payload();
    p->pregenerate();

    //------------------------
    // test specification (1)
    //------------------------
    p->sztype = Fixed;
    cea_gen_spec fxd_spec = {
        .value        = 100,
        .range_start  = 0,
        .range_stop   = 0,
        .range_step   = 0,
        .repeat_after = 0,
        .step         = 0
    }; 
    p->szspec = fxd_spec;

    // p->ptype = Random;
    // p->ptype = Incr_Byte;
    // p->ptype = Incr_Word;
    // p->ptype = Decr_Byte;
    // p->ptype = Decr_Word;
    //
    // p->ptype = Fixed_Pattern;
    // p->pspec.pattern = "aabbccddeeff";
    // 
    // p->ptype = Repeat_Pattern;
    // p->pspec.pattern = "aabbccddeeff";

    p->print_spec();
    p->compute_size_start();
    p->gen_frame();

    p->loop_cnt = 1;
    p->pkts_per_burst = 1;
    p->burst_per_stream = 1;

    p->mutate();


    p->reset();

// //------------------------------------------------------------------------------
// // test specification (1)
// //------------------------------------------------------------------------------
//     p->sztype = Increment;
//     cea_gen_spec inc_spec = {
//         .value        = 0,
//         .range_start  = 70,
//         .range_stop   = 100,
//         .range_step   = 2,
//         .repeat_after = 0,
//         .step         = 0
//     }; 
//     p->szspec = inc_spec;
//     p->print_spec();
//     p->compute_size_start();
//     p->reset();
// 
// //------------------------------------------------------------------------------
// // test specification (2)
// //------------------------------------------------------------------------------
//     p->sztype = Random_in_Range;
//     cea_gen_spec rnd_spec = {
//         .value        = 0,
//         .range_start  = 70,
//         .range_stop   = 100,
//         .range_step   = 0,
//         .repeat_after = 0,
//         .step         = 0
//     }; 
//     p->szspec = rnd_spec;
//     p->print_spec();
//     p->compute_size_start();
//     p->reset();
// 
//     
//     return 0;
}
