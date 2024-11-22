#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {

    string s = "abcd";
    const char *uca = s.c_str();
    cout << uca << endl;
    for(int i=0; i<4; i++) {
        cout << uca[i] << endl;
    }

    vector<char> vs;
    std::copy(s.begin(), s.end(), std::back_inserter(vs));

    for(int i=0; i<4; i++) {
        cout << vs[i] << endl;
    }

    vector<char> vc;
    char *vca = vs.data();

    for(int i=0; i<4; i++) {
        cout << vca[i] << endl;
    }

    return 0;
}
