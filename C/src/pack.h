#ifndef _PACK_H
#define _PACK_H

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
typedef struct PackHead {
	uint8_t  isSync:1;
	uint8_t  dBell:7;
	uint8_t  channel;
	uint16_t sizeData;
	uint32_t indexChunk;
	uint32_t indexPacket;
	uint32_t offsetPacket;
	uint32_t sizePacket;
} PackHead; //20
#pragma pack(pop)

#define DtgMaxSize 1420
#define PackHeadSize sizeof(PackHead)
#define PackInfoSize (DtgMaxSize-PackHeadSize)
#define MaxHolesSync 64

typedef struct ShaPack {
    PackHead head;
    uint8_t data[PackInfoSize];

    MCS      	createdAt;
    MCS      	liveTo;
	int			Used;
    struct ShaPack *predChunk;
    struct ShaPack *nextChunk;
} ShaPack;

#endif
