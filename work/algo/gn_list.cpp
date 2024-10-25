#include "cea.h"

void get_next_value(cea_field_spec &f) {
    f.rt.gen_value = f.spec.value_list[f.rt.list_idx];
    if (f.rt.list_idx == f.spec.value_list.size()-1) {
        if (f.spec.repeat == true) {
            f.rt.list_idx = 0;
        } else {
            f.is_mutable = false;
        }
    } else {
        f.rt.list_idx++;
    }
}

int main() {
    cea_field_spec flen = fdb[MAC_Len];

    cea_gen_spec        len_spec;
    len_spec.repeat     = true;
    // len_spec.repeat     = false;
    len_spec.gen_type   = Value_List;
    len_spec.value_list = {1,2,3,4,5};
    flen.spec           = len_spec;
    flen.is_mutable     = true;

    while (flen.is_mutable) {
        get_next_value(flen);
        cout << flen.rt.gen_value << endl;
    }

    return 0;
}
