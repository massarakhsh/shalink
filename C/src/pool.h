#ifndef _POOL_H
#define _POOL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>

#include "tms.h"
#include "pack.h"

typedef struct ShaPool {
	uint32_t    lastIndexChunk;
    ShaPack    *firstChunk;
    ShaPack    *lastChunk;
} ShaPool;

#endif
