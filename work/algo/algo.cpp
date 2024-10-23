#include "cea.h"


void displayf(cea_field_spec f) {
    switch (f.type) {
        case Integer:
            cout << f.name << " : " << f.spec.value << endl;
            break;
        case Pattern_PRE:
        case Pattern_MAC:
        case Pattern_IPv4:
        case Pattern_IPv6:
            cout << f.name << " : " << f.spec.pattern << endl;
            break;
    }
}

int main() {
    cea_field_spec tpre = fdb[MAC_Preamble];
    cea_field_spec tmac = fdb[MAC_Dest_Addr];
    cea_field_spec tip4 = fdb[IPv4_Src_Addr];
    cea_field_spec tip6 = fdb[IPv6_Src_Addr];
    cea_field_spec tlen = fdb[MAC_Len];

    displayf(tpre);
    displayf(tmac);
    displayf(tip4);
    displayf(tip6);
    displayf(tlen);

    return 0;
}
