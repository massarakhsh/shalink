#ifndef _CHANNEL_H
#define _CHANNEL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include "packet.h"

typedef struct ShaChannel {
    ShaPacket   *firstPacket;
    ShaPacket   *lastPacket;
} ShaChannel;

#endif
