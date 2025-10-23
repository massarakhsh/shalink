#ifndef _CHUNK_H
#define _CHUNK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "tms.h"

#define ChunkProtoPass 0x0
#define ChunkProtoRepeat 0x3
#define ChunkProtoData 0x10

typedef struct ChunkHead {
	uint8_t  proto;
	uint8_t  channel;
	uint16_t sizeData;
	uint32_t indexChannel;
	uint32_t offsetPacket;
	uint32_t sizePacket;
} ChunkHead; //16

#define DtgMaxInput 1500
#define DtgMaxOutput 1420
#define ChunkHeadSize sizeof(ChunkHead)
#define ChunkInfoSize (DtgMaxOutput-ChunkHeadSize)

typedef struct Chunk {
    ChunkHead head;
    uint8_t data[ChunkInfoSize];

    MS       createdAt;
	uint32_t indexChunk;
    Chunk    *predChunk;
    Chunk    *nextChunk;
} Chunk;

#endif
