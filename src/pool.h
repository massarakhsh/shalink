#ifndef _POOL_H
#define _POOL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "chunk.h"
#include "tms.h"

typedef struct ShaPool {
	uint32_t    lastIndexChunk;
    ShaChunk    *firstChunk;
    ShaChunk    *lastChunk;
} ShaPool;

#endif
