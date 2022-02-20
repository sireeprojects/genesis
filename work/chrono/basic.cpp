#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

using namespace std;
using namespace chrono;
  
class runtime {
public:    
    runtime() = default;
    thread tid;

    void worker() {

        auto start = high_resolution_clock::now();

        for (int i=0; i<1'000'000'000ull; i++) {
        }

        auto end = high_resolution_clock::now();
        // Calculating total time taken by the program.
        double time_taken = duration_cast<nanoseconds>(end - start).count();
        time_taken *= 1e-9;
        cout << endl << "Time taken by program is : " << 
            fixed << setprecision(3) << time_taken  << " sec";
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
