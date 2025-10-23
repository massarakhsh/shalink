#ifndef _POOL_H
#define _POOL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "chunk.h"
#include "tms.h"

typedef struct Pool {
	uint32_t indexChunk;
    Chunk    *firstChunk;
    Chunk    *lastChunk;
} Pool;

#endif
