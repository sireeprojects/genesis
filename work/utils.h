#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
using namespace std;
using namespace chrono;


#define CEA_FORMATTED_HDR_LEN 80
string formatted_hdr(string s) {
    stringstream ss;
    ss.setf(ios_base::left);
    ss << string(3,'-') << "{ ";
    ss << s << " }" << string((CEA_FORMATTED_HDR_LEN-(7+s.length())),'-');
    ss << endl;
    return ss.str();
}

void heading(string s) {
    cout << formatted_hdr(s) << endl;
}

class cea_timer {
public:
    cea_timer() = default;

    void start() {
       begin = high_resolution_clock::now();
    }
    double elapsed() {
        end = high_resolution_clock::now();
        double delta = duration<double>(end-begin).count();
        return delta;
    }
    string elapsed_in_string() {
        int precision = 3;
        end = high_resolution_clock::now();
        double delta = duration<double>(end-begin).count();
        stringstream ss;
        ss << fixed << setprecision(precision) << delta << " sec";
        return ss.str();
    }
private:
    time_point<high_resolution_clock> begin;
    time_point<high_resolution_clock> end;
};
