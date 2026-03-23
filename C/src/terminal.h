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
#include "queue.h"
#include "pack.h"
#include "guest.h"
#include "frame.h"
#include "pool.h"
#include "channel.h"
#include "statistic.h"
#include "link.h"

#define MaxChannelCount 256

typedef struct ShaTerminal {
    char name[256];
    pthread_t hThread;
    pthread_mutex_t mutex;
    ShaStatistic statistic;

    MCS      latency;

    int      isStoping;
    int      isStoped;
    int      isMirror;
    ShaChannel inputChannel[MaxChannelCount];
    uint32_t   indexPacket[MaxChannelCount];

    ShaLink  *firstLink;
    ShaLink  *lastLink;
    ShaPool  inputPool;
    ShaPool  outputPool;

    MCS   stepPause;
    uint32_t lastDoneChunk;
    uint32_t lastInputChunk;
} ShaTerminal;

#ifdef __cplusplus
extern "C" {
#endif

ShaTerminal* BuildTerminal(const char *name);
void TerminalSend(ShaTerminal *terminal, MCS latency, uint8_t channel, const void *data, uint32_t size);
ShaLink* TerminalAddLink(ShaTerminal *terminal, const char *address, int port, int isServer);
ShaFrame* TerminalGetChannel(ShaTerminal *terminal, uint8_t channel);
ShaFrame* TerminalGetPacket(ShaTerminal *terminal);
void TerminalSetLatency(ShaTerminal *terminal, MCS latency);
int TerminalIsReady(ShaTerminal *terminal);
int TerminalFree(ShaTerminal *terminal);
int TerminalStop(ShaTerminal *terminal);
void PacketFree(ShaFrame *packet);

#ifdef __cplusplus
}
#endif

#endif
