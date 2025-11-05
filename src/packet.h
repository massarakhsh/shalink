#ifndef _PACKET_H
#define _PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "tms.h"
#include "chunk.h"

typedef struct ShaPacket {
	uint8_t channel;
	uint32_t indexPacket;
	uint32_t sizeData;
	uint32_t sizeDone;
    uint8_t *data;

	MCS		createdAt;
	MCS		appendAt;
	MCS		storedAt;

	ShaPacket 	 *predPacket;
	ShaPacket   *nextPacket;
} ShaPacket;

#endif
