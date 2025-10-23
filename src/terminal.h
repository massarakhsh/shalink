#ifndef _TERMINAL_H
#define _TERMINAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <poll.h>

#include "tms.h"
#include "link.h"
#include "packet.h"
#include "chunk.h"
#include "pool.h"

#define MaxChannelCount 256

typedef struct Terminal {
    char name[256];
    pthread_t hThread;
    pthread_mutex_t mutex;

    MS       latency;
    int      isStoping;
    int      isStoped;
    uint32_t indexOutput[MaxChannelCount];
    uint32_t indexInput[MaxChannelCount];

    MS      stepAt;
    int     stepPauseMcs;
    Packet  *stepPacket;

    Link    *firstLink;
    Link    *lastLink;
    Packet  *firstPacket;
    Packet  *lastPacket;
    Pool    inputPool;
    Pool    outputPool;
} Terminal;

Terminal* BuildTerminal(const char *name);
void TerminalSend(Terminal *terminal, uint8_t channel, const void *data, uint16_t size);
Link* TerminalAddLink(Terminal *terminal, const char *address, int port, int isServer);
Packet* TerminalGet(Terminal *terminal);
void TerminalFree(Terminal *terminal);
void TerminalStop(Terminal *terminal);
void PacketFree(Packet *packet);

#endif
