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


}
