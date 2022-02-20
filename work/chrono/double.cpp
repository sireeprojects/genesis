#include "../utils.h"
#include <thread>

using namespace std;
using namespace chrono;

class runtime {
public:    
    runtime() = default;
    thread tid;
    cea_timer timer;

    void worker() {
        timer.start();
        for (int i=0; i<1'000'000'000ull; i++) {
        }
        cout << "Time taken by program is : " << fixed << setprecision(3) 
            << timer.elapsed()  << " sec" << endl;
        cout << timer.elapsed_in_string() << endl;
    }

    void start() {
        tid = thread(&runtime::worker, this);
        // tid.detach();
    }
    void join_threads() {
        tid.join();
    }
};

int main() {
    runtime r1, r2;
    r1.start();
    r2.start();

    r1.join_threads();
    r2.join_threads();
  
    cout << endl;

    return 0;
}

// unsync the I/O of C and C++.
// ios_base::sync_with_stdio(false);
