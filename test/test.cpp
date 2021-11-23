#include "cdn_eth_proxy.h"
using namespace cdn_eth_avip;


int main() {
    cdn_eth_stream s;

    // set field values
    s.set(CEA_MAC_Preamble,10);
    cdn_eth_logger << s.describe();
    return 0;
}
