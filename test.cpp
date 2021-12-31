#include "cea.h"
using namespace cea;

int main() {
    cea_manager *mgr = new cea_manager;

    cea_proxy *s0 = new cea_proxy;
    cea_proxy *s1 = new cea_proxy;
    cea_proxy *s2 = new cea_proxy;
    cea_proxy *s3 = new cea_proxy;
    cea_proxy *s4 = new cea_proxy;

    mgr->add_proxy(s0);
    mgr->add_proxy(s1);
    mgr->add_proxy(s2);
    mgr->add_proxy(s3);
    mgr->add_proxy(s4);

    mgr->testfn(s0);
    mgr->testfn(s1);
    mgr->testfn(s2);
    mgr->testfn(s3);
    mgr->testfn(s4);

    return 0;
}

// int main() {
//     cea_manager *mgr = new cea_manager;
//     cea_proxy *pxy = new cea_proxy;
//     cea_stream *stm = new cea_stream;
// 
//     uint32_t a = 10;
// 
//     CEA_MSG("Just a testbench message %d", a);
// 
//     mgr->add_proxy(pxy);
//     pxy->add_stream(stm);
// 
//     cea_proxy p[10];
//     mgr->add_proxy(p, 10);
// 
//     cea_proxy *pp = new cea_proxy[10];
//     mgr->add_proxy(pp, 10);
// 
//     return 0;
// }






