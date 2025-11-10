#ifndef _GUEST_H
#define _GUEST_H

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

typedef struct ShaGuest {
    struct sockaddr_in addr;
    char saddr[64];
    ShaBrief brief;
    ShaQueue queue;

    ShaGuest *predGuest;
    ShaGuest *nextGuest;
} ShaGuest;

#endif
