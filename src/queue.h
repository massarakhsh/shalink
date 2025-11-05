#ifndef _QUEUE_H
#define _QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "chunk.h"
#include "tms.h"

#define MaxQueuePots 1024

typedef struct ShaPot {
	uint32_t firstChunk;
	uint32_t countChunk;
} ShaPot;

typedef struct ShaQueue {
    int      countPots;
    ShaPot   pots[MaxQueuePots];
} ShaQueue;

#endif
