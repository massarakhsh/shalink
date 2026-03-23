#ifndef _CHUNK_H
#define _CHUNK_H

#ifdef __cplusplus
extern "C" {
#endif

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

#pragma pack(push, 1)
typedef struct ChunkHead {
	uint8_t  isSync:1;
	uint8_t  dBell:7;
	uint8_t  channel;
	uint16_t sizeData;
	uint32_t indexChunk;
	uint32_t indexPacket;
	uint32_t offsetPacket;
	uint32_t sizePacket;
} ChunkHead; //20
#pragma pack(pop)

#define DtgMaxSize 1420
#define ChunkHeadSize sizeof(ChunkHead)
#define ChunkInfoSize (DtgMaxSize-ChunkHeadSize)
#define MaxHolesSync 64

typedef struct ShaChunk {
    ChunkHead head;
    uint8_t data[ChunkInfoSize];

    MCS      	createdAt;
    MCS      	liveTo;
	int			Used;
    struct ShaChunk *predChunk;
    struct ShaChunk *nextChunk;
} ShaChunk;

#ifdef __cplusplus
}
#endif

#endif
