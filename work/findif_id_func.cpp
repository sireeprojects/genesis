#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <iomanip>

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

void print_fields() {
    cout << "Field Values" << endl << string(45, '-') << endl;
    for (auto f : fields) {
        cout << setw(5) << f.touched
             << setw(5) << f.merge
             << setw(5) << f. mask
             << setw(5) << f.id
             << setw(5) << dec << f.len
             << setw(20) << hex << f.value << endl;
    }
}

using fit = vector<cea_field>::iterator;

fit getit(vector<cea_field> *table, unsigned id) {
    auto lit = find_if(table->begin(), table->end(),
                    [&id](const cea_field &item) {
                    return (item.id == id); 
                    });
    if (lit == table->end())
        abort();

    return lit; 
}

int main() {

    print_fields();

    auto t = getit(&fields, 6);
    cout << t->len << endl;
    t->len=30;

    print_fields();

    return 0;
}
