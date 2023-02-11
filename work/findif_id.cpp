#include <iostream>
#include <vector>
#include <algorithm>
#include <string>

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

vector<unsigned> ids = {2,3,4};

int main() {

    // search multiple ids
    for (auto id : ids) {
        auto lit = find_if(fields.begin(), fields.end(),
                        [&id](const cea_field &item) {
                        return (item.id == id); 
                        });
        cout << "match is " << hex << (*lit).value << endl;
    }

    // search single id
    unsigned id = 6;
    auto lit = find_if(fields.begin(), fields.end(),
                    [&id](const cea_field &item) {
                    return (item.id == id); 
                    });
    if (lit != fields.end())
        cout << "match is " << hex << (*lit).value << endl;

    return 0;
}
