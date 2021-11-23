#include "cea.h"
using namespace cea;


int main() {
    ceaLog << "Size of Single Field: " << dec << sizeof(field) << " bytes" << endl;
    ceaLog << "Size of Single Stream: " << dec << sizeof(stream) << " bytes" << endl;
    return 0;
}
