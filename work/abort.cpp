#include <iostream>
#include <csignal>

using namespace std;

void signal_handler(int signal) 
{
    if (signal == SIGABRT) {
        cerr << "SIGABRT received\n";
    } else {
        cerr << "Unexpected signal " << signal << " received\n";
    }
    exit(EXIT_FAILURE);
}

int main() {
    signal(SIGABRT, signal_handler);
    abort();
    return 0;
}
