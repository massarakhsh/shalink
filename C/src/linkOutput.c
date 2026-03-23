#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

#define MaxStepOutputChunk 256

int getSocketError(int sockfd) {
    int error = 0;
    socklen_t len = sizeof(error);   
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        return errno;
    }
    return error;
}

void shaLinkOutputQueue(ShaLink *link, uint32_t firstChunk, uint32_t countChunk) {
    if (link->isServer) {
        for (ShaGuest *guest = link->firstGuest; guest != NULL; guest = guest->nextGuest) {
            shaQueueInsert(&guest->queue, firstChunk, countChunk);
        }
    } else {
        shaQueueInsert(&link->queue, firstChunk, countChunk);
    }
}

ssize_t shaLinkOutputCode(ShaLink *link, ShaGuest *guest, const void *code, int size) {
    socklen_t client_len = sizeof(guest->addr);
    ssize_t sent = 0;
    sent = sendto(link->socketId, code, size, 0, (struct sockaddr*)&(guest->addr), client_len);
    return sent;
}

void shaLinkOutputGuest(ShaLink *link, ShaGuest *guest, ShaPack *chunk) {
    ShaTerminal *terminal = link->terminal;
    ssize_t size = PackHeadSize + chunk->head.sizeData;
    ssize_t sent = 0;
    if (chunk->head.isSync) {
        terminal->statistic.syncsOut++;
    } else {
        chunk->Used++;
        if (chunk->Used == 1) {
            terminal->statistic.packetsOut++;
            terminal->statistic.packetsOut1++;
        } else if (chunk->Used == 2) {
            terminal->statistic.packetsOut1--;
            terminal->statistic.packetsOut2++;
        } else if (chunk->Used == 3) {
            terminal->statistic.packetsOut2--;
            terminal->statistic.packetsOut3++;
        }
    }
    terminal->statistic.pksTotalOut++;
    terminal->statistic.btsTotalOut += size;

    if (guest != NULL) {
        sent = shaLinkOutputCode(link, guest, chunk,  size);
    } else {
        sent = send(link->socketId, chunk, size, 0);
    }
    if (sent < 0) {
        //printf("SEND ERROR: %ld\n", sent);
    }
}

void linkOutputStep(ShaLink *link, ShaGuest *guest) {
    ShaTerminal *terminal = link->terminal;
    int inWork = 0;
    while (inWork < MaxStepOutputChunk) {
        ShaQueue *queue = (guest != NULL) ? &guest->queue : &link->queue;
        if (!shaQueuePresent(queue)) break;
        uint32_t indexChunk = shaQueueGet(queue);
        for (ShaPack *chunk = terminal->outputPool.lastChunk; chunk != NULL; chunk = chunk->predChunk) {
            int cmp = shaCompare(indexChunk, chunk->head.indexChunk);
            if (cmp == 0) {
                shaLinkOutputGuest(link, guest, chunk);
                inWork++;
            }
            if (cmp >= 0) break;
        }
    }
}

void shaLinkOutput(ShaLink *link) {
    if (link->isServer) {
        for (ShaGuest *guest = link->firstGuest; guest != NULL; guest = guest->nextGuest) {
            linkOutputStep(link, guest);
        }
    } else {
        linkOutputStep(link, NULL);
    }
}
