#ifndef _QUEUE_H
#define _QUEUE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "chunk.h"
#include "tms.h"

typedef struct ShaPot {
	uint32_t firstChunk;
	uint32_t countChunk;

    ShaPot *predPot;
    ShaPot *nextPot;
} ShaPot;

typedef struct ShaQueue {
    ShaPot *firstPot;
    ShaPot *currentPot;
    ShaPot *lastPot;
} ShaQueue;

#endif
