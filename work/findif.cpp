#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#define BREAK cout << string(80, '-') << endl;
using namespace std;

struct cea_field {
    bool touched;
    uint32_t merge;
    uint64_t mask;
    uint32_t id;
    uint32_t len;
    uint64_t value;
};

vector <cea_field> fields = {
// Toc  Mrg  Mask Id   Len  Value    
{  0,   0,   0,   0,   4,   0x11223344              },
{  0,   6,   0,   1,   1,   4                       },
{  0,   0,   0,   2,   1,   5                       },
{  0,   0,   0,   3,   1,   0xff                    },
{  0,   0,   0,   4,   1,   0xccdd                  },
{  0,   0,   0,   5,   1,   0xee                    },
{  0,   0,   0,   6,   3,   0xff                    },
{  0,   0,   0,   7,   8,   0x12345678aabbccddull   }
};

bool check(cea_field f) {
    if (f.len==8) return true;
    return false;
}

int main() {
    cout << "find_if test" << endl;

    // predicate is a function
    vector<cea_field>::iterator it;
    it = find_if(fields.begin(), fields.end(), check);
    cout << "match is " << hex << (*it).value << endl;

    // predicate is a lambda
    auto lit = std::find_if( fields.begin(), fields.end(),
            [](const cea_field &item) {
                return (item.len == 8); 
            });
    cout << "match is " << hex << (*lit).value << endl;

    return 0;
}
