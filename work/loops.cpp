#include <iostream>
#include <cstdint>
using namespace std;

string toc(uint32_t len, string msg) {
    string rt = msg.erase(msg.find_last_not_of(" \t\n\r\f\v") + 1);
    string lt = rt.erase(0, rt.find_first_not_of(" \t\n\r\f\v"));
    string s = lt + string((len - lt.length()), '.') + " ";
    return s;
}

int main() {

    uint32_t burst_per_stream = 1;
    uint32_t pkts_per_burst = 1;

    uint32_t burst_control = 1;
    uint32_t pkt_control = 1;

    uint32_t frm_cnt = 0;
    uint32_t burst_cnt = 0;

    uint32_t nof_sizes = 4;

    for(uint32_t bps=0; bps<burst_per_stream; bps+=burst_control) 
    {
        for(uint32_t ppb=0; ppb<pkts_per_burst; ppb+=pkt_control) 
        {
            // for(uint32_t frm_cnt=0; frm_cnt<10; frm_cnt++) {
            //     cout << "i=" << frm_cnt << "   " 
            //          << "size[" << (frm_cnt%nof_sizes) << "]"
            //          << endl;
            // }
            frm_cnt++;
            cout << toc(30, "Frame cnt ") << frm_cnt << endl;
        }
        burst_cnt++;
        cout << toc(50, "Burst complete") << burst_cnt << endl;
    }

    cout << endl;
    cout << toc(30, "Number of Burst ") << burst_per_stream << endl;
    cout << toc(30, "Number of Packets ") << pkts_per_burst << endl;

    return 0;
}
