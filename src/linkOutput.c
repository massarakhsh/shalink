#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

#define MaxStepOutputChunk 64

int getSocketError(int sockfd) {
    int error = 0;
    socklen_t len = sizeof(error);   
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
        return errno;
    }
    return error;
}

void shaLinkOutputGuest(ShaLink *link, ShaGuest *guest, ShaChunk *chunk) {
    ShaTerminal *terminal = link->terminal;
    ssize_t size = ChunkHeadSize + chunk->head.sizeData;
    ssize_t sent = 0;
    if (chunk->head.proto == ChunkProtoData) {
        if (chunk->Used == 0) terminal->statistic.sendDataChunks++;
        else terminal->statistic.sendRepeatChunks++;
        chunk->Used++;
    } else if (chunk->head.proto == ChunkProtoSync) {
        terminal->statistic.sendSyncChunks++;
    }
    terminal->statistic.sendTotalChunks++;
    terminal->statistic.sendTotalBytes += size;

    if (chunk->head.proto == ChunkProtoData) {       
        //printf("Output chunk %d\n", chunk->head.indexChunk);
    }

    int error = getSocketError(link->socketId);
    if (error != 0) printf("SOCKET ERROR: %d\n", error);
    if (guest != NULL) {
        socklen_t client_len = sizeof(guest->addr);
        sent = sendto(link->socketId, chunk, size, 0, (struct sockaddr*)&(guest->addr), client_len);
    } else {
        sent = send(link->socketId, chunk, size, 0);
    }
    if (sent <= 0) printf("SEND ERROR: %ld\n", sent);
}

void linkOutputStep(ShaLink *link, ShaGuest *guest) {
    ShaTerminal *terminal = link->terminal;
    int inWork = 0;
    while (inWork < MaxStepOutputChunk) {
        ShaQueue *queue = (guest != NULL) ? &guest->queue : &link->queue;
        if (!shaQueuePresent(queue)) break;
        uint32_t indexChunk = shaQueueGet(queue);
        for (ShaChunk *chunk = terminal->outputPool.lastChunk; chunk != NULL; chunk = chunk->predChunk) {
            int cmp = shaCompare(indexChunk, chunk->head.indexChunk);
            if (cmp > 0) break;
            if (cmp == 0) {
                shaLinkOutputGuest(link, guest, chunk);
                inWork++;
            }
        }
    }
}

void linkOutputQueue(ShaLink *link, uint32_t firstChunk, uint32_t countChunk) {
    if (link->isServer) {
        for (ShaGuest *guest = link->firstGuest; guest != NULL; guest = guest->nextGuest) {
            shaQueueInsert(&guest->queue, firstChunk, countChunk);
        }
    } else {
        shaQueueInsert(&link->queue, firstChunk, countChunk);
    }
}

void shaLinkOutput(ShaLink *link) {
    ShaTerminal *terminal = link->terminal;
    uint32_t lastIndexChunk = terminal->outputPool.lastIndexChunk;
    if (shaCompare(lastIndexChunk, link->brief.lastQueueChunk) > 0) {
        uint32_t first = link->brief.lastQueueChunk+1;
        uint32_t count = lastIndexChunk+1-first;
        linkOutputQueue(link, first, count);
        link->brief.lastQueueChunk = lastIndexChunk;
    }
    if (link->isServer) {
        for (ShaGuest *guest = link->firstGuest; guest != NULL; guest = guest->nextGuest) {
            linkOutputStep(link, guest);
        }
    } else {
        linkOutputStep(link, NULL);
    }
}
