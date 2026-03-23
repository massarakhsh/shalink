#ifndef _FRAME_H
#define _FRAME_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "tms.h"
#include "pack.h"

typedef struct ShaFrame {
	uint8_t channel;
	uint32_t indexPacket;
	uint32_t sizeData;
	uint32_t sizeDone;
    uint8_t *data;

	MCS		createdAt;
	MCS		appendAt;
	MCS		storedAt;
	MCS		liveAt;

	struct ShaFrame *predFrame;
	struct ShaFrame *nextFrame;
} ShaFrame;

#endif
