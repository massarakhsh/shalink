#ifndef _LINK_H
#define _LINK_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include "tms.h"
#include "sync.h"
#include "queue.h"
#include "guest.h"

#define LinkBufferSize 1500

struct ShaTerminal;

typedef struct ShaLink {
    struct ShaTerminal *terminal;
    char address[256];
    int  port;
    int  isServer;

    int      socketId;
    MCS      socketAt;
    int      isBinded;
    int      isOpened;
    uint8_t  in_buffer[LinkBufferSize];
    ShaBrief brief;
    ShaQueue queue;
    ShaGuest *firstGuest;
    ShaGuest *lastGuest;

    ShaLink *predLink;
    ShaLink *nextLink;
} ShaLink;

#endif
