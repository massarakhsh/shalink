#ifndef _STATISTIC_H
#define _STATISTIC_H

#include <stdio.h>
#include <stdlib.h>
#include "tms.h"

typedef struct shaStatistic {
    int64_t framesOut;
    int64_t framesIn;
    int64_t framesLost;
    int64_t framesBreak;

    int64_t packetsOut;
    int64_t packetsOut1;
    int64_t packetsOut2;
    int64_t packetsOut3;
    int64_t packetsIn;
    int64_t packetsDouble;
    int64_t packetsLate;

    int64_t syncsOut;
    int64_t syncsIn;

    int64_t bytesOut;
    int64_t bytesIn;

    int64_t pksTotalIn;
    int64_t pksTotalOut;
    int64_t btsTotalIn;
    int64_t btsTotalOut;

    MCS     meRTT;
    MCS     youRTT;
} ShaStatistic;

#endif

