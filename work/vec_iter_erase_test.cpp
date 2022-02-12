#include <iostream>
#include <vector>
#include <string>
#define BREAK cout << string(80, '-') << endl;
using namespace std;

int main() {

    vector<int> q;

    for (int i=0; i<10; i++)
        q.push_back(i);

// iterate methods -------------------------------------------------------------
    
    // interate method:1
    // vector<int>::iterator it;
    // for (it=q.begin(); it!=q.end(); it++) cout << *it << endl;
    // BREAK;
    
    // iterate method:2
    // for (auto i=q.begin(); i!=q.end(); i++) cout << *i << endl;
    // BREAK;
  
    // iterate method:3
    for (auto &i : q) cout << i << endl;
    BREAK;

// delete methods --------------------------------------------------------------

    // delete single element
    q.erase(q.begin());
    for (auto &i : q) cout << i << endl;
    BREAK;

    // delete multiple elements
    q.erase(q.begin(), q.begin()+3);
    for (auto &i : q) cout << i << endl;
    BREAK;

    // delete current element
    q.erase(q.begin());
    for (auto &i : q) cout << i << endl;
    BREAK;

    // delete only next element
    q.erase(next(q.begin()));
    for (auto &i : q) cout << i << endl;
    BREAK;

    // delete nth element from current position
    q.erase(next(q.begin(), 2));
    for (auto &i : q) cout << i << endl;
    BREAK;

    return 0;
}
