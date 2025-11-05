#ifndef _STATISTIC_H
#define _STATISTIC_H

#include <stdio.h>
#include <stdlib.h>
#include "tms.h"

typedef struct ShaStatistic {
    int64_t sendDataPackets;
    int64_t sendDataBytes;
    int64_t sendDataChunks;
    int64_t sendRepeatChunks;
    int64_t sendSyncChunks;
    int64_t sendTotalPackets;
    int64_t sendTotalChunks;
    int64_t sendTotalBytes;

    int64_t recvDataPackets;
    int64_t recvDataBytes;
    int64_t recvDataChunks;
    int64_t recvRepeatChunks;
    int64_t recvSyncChunks;
    int64_t recvTotalPackets;
    int64_t recvTotalChunks;
    int64_t recvTotalBytes;

    MCS     meRTT;
    MCS     youRTT;
} ShaStatistic;

#endif
