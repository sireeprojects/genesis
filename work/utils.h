#include <iostream>
#include <string>
#include <sstream>
using namespace std;


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
