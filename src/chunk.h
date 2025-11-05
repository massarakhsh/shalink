#ifndef _CHUNK_H
#define _CHUNK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "tms.h"
#include "sync.h"

#define ChunkProtoPass 0x0
#define ChunkProtoSync 0x1
#define ChunkProtoRequest 0x3
#define ChunkProtoData 0x10

typedef struct ChunkHead {
	uint8_t  proto;
	uint8_t  channel;
	uint16_t sizeData;
	uint32_t indexChunk;
	uint32_t indexPacket;
	uint32_t offsetPacket;
	uint32_t sizePacket;
} ChunkHead; //20

#define DtgMaxSize 1420
#define ChunkHeadSize sizeof(ChunkHead)
#define ChunkInfoSize (DtgMaxSize-ChunkHeadSize)
#define MaxHolesSync 64

typedef struct ShaChunk {
    ChunkHead head;
    uint8_t data[ChunkInfoSize];

    MCS      createdAt;
	int			Used;
    ShaChunk    *predChunk;
    ShaChunk    *nextChunk;
} ShaChunk;

#endif
