#include "payload.h"
using namespace std;

int main() {
    payload *p = new payload();
    p->pregenerate();

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
// //------------------------------------------------------------------------------
// // test specification (3)
// //------------------------------------------------------------------------------
//     p->sztype = Fixed;
//     cea_gen_spec fxd_spec = {
//         .value        = 100,
//         .range_start  = 0,
//         .range_stop   = 0,
//         .range_step   = 0,
//         .repeat_after = 0,
//         .step         = 0
//     }; 
//     p->szspec = fxd_spec;
//     p->print_spec();
//     p->compute_size_start();
//     p->reset();
//     
//     return 0;
}
