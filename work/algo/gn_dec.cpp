#include "cea.h"
#include <unistd.h>

void get_next_value(cea_field_spec &f) {
    f.rt.gen_value = f.rt.value_in_range;

    // if end of range is reached
    if((f.rt.gen_value == f.spec.min) || ( f.spec.step >= f.rt.gen_value)) {
        // cout << "end of range" << endl;
        if (f.spec.repeat == true) { // if repeat is set
            // cout << "roll over" << endl;
            f.rt.value_in_range = f.spec.max; // rool over
        } else {
            // cout << "remove from mutables" << endl;
            f.is_mutable = false; // remove from mutables
        }
    } else { // assign next value in range
        // cout << "next value in range" << endl;
        f.rt.value_in_range = f.rt.value_in_range - f.spec.step;
    }
}

int main() {
    cea_field_spec flen = fdb[MAC_Len];

    cea_gen_spec        len_spec;
    // len_spec.repeat     = true;
    len_spec.repeat     = false;
    len_spec.gen_type   = Decrement;
    len_spec.step       = 10;
    len_spec.max        = 10;
    len_spec.min        = 1;
    flen.spec           = len_spec;
    flen.is_mutable     = true;

    // prepare field runtime, to be done inside set()
    flen.rt.value_in_range = len_spec.max;

    while (flen.is_mutable) {
        get_next_value(flen);
        cout << flen.rt.gen_value << endl;
        sleep(1);
    }

    return 0;
}
