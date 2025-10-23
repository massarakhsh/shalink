#ifndef _PACKET_H
#define _PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "tms.h"
#include "chunk.h"

typedef struct Packet {
	uint8_t channel;
	uint32_t indexChannel;
	uint32_t sizeData;
	uint32_t sizeDone;
    uint8_t *data;
	uint8_t isDone;
	MS		createdAt;
	MS		appendedAt;

    Chunk    *firstChunk;
    Chunk    *lastChunk;

	uint8_t		holeOffset;
	uint8_t		holeSize;

	Packet 	 *predPacket;
	Packet   *nextPacket;
} Packet;

#endif
