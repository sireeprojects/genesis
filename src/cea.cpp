#include "cea.h"


namespace cea {

ofstream logfile;
msgVerbosityLevel global_verbosity;

int outbuf::overflow(int_type c) {
    if (c != EOF) {
	c = static_cast<char>(c);
        logfile << static_cast<char>(c);
        if (putchar(c) == EOF) {
           return EOF;
        }
    }
    return c;
}


class initLib {
public:
    initLib() {
        logfile.open("run.log", ofstream::out);
        if (!logfile.is_open()) {
            cout << "Error creating logfile. Aborting..." << endl;
            exit(1);
        }
    }
    ~initLib() { logfile.close(); }
};


initLib init_lib;


}
