#ifndef _CONNECT_H
#define _CONNECT_H

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

#define MaxGuestCount 64

struct Terminal;

typedef struct Guest {
    struct sockaddr_in addr;
    int32_t indexChunk;
    MS lastIn;
} Guest;

typedef struct Link {
    struct Terminal *terminal;
    char address[256];
    int  port;
    int  isServer;

    int  socketId;
    MS   socketAt;
    int  isBinded;
    int  isOpened;
    MS   sendAt;
    int  guestCount;
    Guest guests[MaxGuestCount];
    int32_t indexChunk;

    Link *predLink;
    Link *nextLink;
} Link;

#endif
