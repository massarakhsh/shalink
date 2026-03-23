#ifndef _CHANNEL_H
#define _CHANNEL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>

#include "frame.h"

typedef struct ShaChannel {
    ShaFrame   *firstFrame;
    ShaFrame   *lastFrame;
} ShaChannel;

#endif
