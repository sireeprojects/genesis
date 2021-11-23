#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <iomanip>
#include <fstream>
#include <locale>
#include <cstdio>
#include <streambuf>
#define PACKED __attribute__((packed))
using namespace std;


namespace cea {


class outbuf : public streambuf {
protected:
    virtual int overflow(int c) ;
};
outbuf ob;
ostream ceaLog(&ob);


typedef enum {
    ceaLow,
    ceaFull,
} msgVerbosityLevel;


typedef enum {
    ceaV2,
    ceaRaw
} frameType;


typedef enum {
    ceaMacPreamble,
    ceaMacDestAddr,
    ceaMacSrcAddr,
    ceaMacLen,
    ceaMacEtherType,
    ceaMacFcs,
    ceaIpv4Version,
    ceaIpv4IHL,
    ceaIpv4Tos,
    ceaIpv4totalLen,
    ceaIpv4Id,
    ceaIpv4Flags,
    ceaIpv4FragOffset,
    ceaIpv4TTL,
    ceaIpv4Protocol,
    ceaIpv4HdrCsum,
    ceaIpv4SrcAddr,
    ceaIpv4DestAddr,
    ceaIpv4Opts,
    ceaIpv4Pad,
    ceaUDPSrcPort,
    ceaUDPDestPort,
    ceaUDPlen,
    ceaUDPCsum,
    ceaStreamType,
    ceaStreamPktsPerBurst,
    ceaStreamBurstPerStream,
    ceaStreamInterBurstGap,
    ceaStreamInterStreamGap,
    ceaStreamStartDelay,
    ceaStreamRateType,
    ceaStreamIpg,
    ceaStreamPercentage,
    ceaStreamPktsPerSec,
    ceaStreamBitRate,
    ceaPayloadType,
    ceaFieldsSize
} fieldId;


typedef enum {
    // value modifiers
    fixed,                  // init value
    random,                 // from 0 to max int
    rrandom,                // random in range
    increment,              // from start to stop with steps
    decrement,              // from start to stop with steps
    cincrement,             // starts from 0 and loops at repeat after
    cdecrement,             // starts from ff and loops at repeat after
    // data modifiers
    incrByte,              // start from zero
    incrWord,              // start from zero
    decrByte,              // start from FF
    decrWord,              // start from FF
    repeattPattern,        // repeat a data pattern
    fixedPattern,          // use data pattern only once, rest 0s
    // stream modifiers
    continuousPkts,            
    continuousBurst,          
    stopAfterStream,    
    gotoNextStream,
    // stream rate modifiers
    ipg,
    percentage,
    pktsPerSec,
    bitRate
} fieldModifier;


struct PACKED field { // 81 bytes
    bool touched : 1;
    bool merge : 1;
    uint64_t mask : 64;
    uint32_t id : 32;
    uint32_t len: 32;
    uint32_t ofset : 32;
    fieldModifier modifier : 32;
    uint64_t value: 64;
    uint32_t start: 32;
    uint32_t stop: 32;
    uint32_t step: 32;
    uint32_t repeat: 32;
    char name[32];
    char pad[47];
};

struct PACKED stream {
    field fields[64];
};

}
