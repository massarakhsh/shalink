#ifndef _CHUNK_H
#define _CHUNK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "tms.h"

#define ChunkProtoPass 0x0
#define ChunkProtoConf 0x1
#define ChunkProtoConfTo 0x2
#define ChunkProtoRep 0x3
#define ChunkProtoRepFrom 0x4
#define ChunkProtoData 0x10

typedef struct ChunkHead {
	uint8_t  proto;
	uint8_t  channel;
	uint16_t sizeData;
	uint32_t indexChunk;
	uint32_t indexChannel;
	uint32_t offsetPacket;
	uint32_t sizePacket;
} ChunkHead; //20

#define DtgMaxInput 1500
#define DtgMaxOutput 30
#define ChunkHeadSize sizeof(ChunkHead)
#define MaxChunkSize (DtgMaxOutput-ChunkHeadSize)

typedef struct Chunk {
    ChunkHead head;
    uint8_t data[MaxChunkSize];

    MS       createdAt;
    Chunk    *predChunk;
    Chunk    *nextChunk;
} Chunk;

#endif
