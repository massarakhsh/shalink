#ifndef _SYNC_H
#define _SYNC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include "tms.h"

#define MaxRtts 8

typedef struct ShaSyncPair {
	uint32_t needYouFirst;
	uint32_t needYouCount;
} ShaSyncPair;

typedef struct ShaSync {
	uint64_t meTime;
	uint64_t youTime;
	uint64_t mePause;
    uint64_t meRTT;
	uint32_t indexYouRead;
	uint32_t indexMyFirst;
	uint32_t indexMyCount;
	uint32_t countHoles;
//	list of SyncPairs;
} ShaSync;

typedef struct ShaBrief {
    int32_t  lastQueueChunk;
    MCS   lastInputMs;
    MCS   lastSync;
    MCS   youTime;
    MCS   meTime;
    MCS   rtts[MaxRtts];
    MCS   rttAvg;
    ShaSync  youSync;
} ShaBrief;

#endif
