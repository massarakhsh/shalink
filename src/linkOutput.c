#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

MS KeepAliveMs = 100;

void linkOutputServer(Link *link, Guest *guest) {
    Chunk *chunk = link->terminal->outputPool.firstChunk;
    int inWork = 0;
    while (chunk != NULL && inWork < 256) {
        uint32_t chi = chunk->head.indexChunk;
        if (ioCompareIndex(guest->indexChunk, chi) < 0) {
            int size = ChunkHeadSize + chunk->head.sizeData;
            socklen_t client_len = sizeof(guest->addr);
            ssize_t sent = sendto(link->socketId, chunk, size, 0, (struct sockaddr*)&(guest->addr), client_len);
            if (sent != size) {
                printf("Send size error: %ld/%d\n", sent, size);
            }
            if (sent < 0) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    printf("Real error\n");
                    break;
                }
            }
            guest->indexChunk = chi;
            link->terminal->stepPauseMcs = 0;
            inWork++;
        }
        chunk = chunk->nextChunk;
    }
}

void linkOutputClient(Link *link) {
    Chunk *chunk = link->terminal->outputPool.firstChunk;
    int inWork = 0;
    while (chunk != NULL && inWork < 256) {
        uint32_t chi = chunk->head.indexChunk;
        if (ioCompareIndex(link->indexChunk, chi) < 0) {
            if (send(link->socketId, chunk, ChunkHeadSize + chunk->head.sizeData, 0) < 0) {
                if (errno != EWOULDBLOCK && errno != EAGAIN) {
                    printf("Реальная ошибка\n");
                    break;
                }
            }
            link->indexChunk = chi;
            link->terminal->stepPauseMcs = 0;
            inWork++;
        }
        chunk = chunk->nextChunk;
    }
}

void linkOutput(Link *link) {
    if (link->isServer) {
        for (int g = 0; g < MaxGuestCount; g++) {
            Guest *guest = link->guests+g;
            if (guest->lastIn > 0) {
                linkOutputServer(link, guest);
            }
        }
    } else {
        linkOutputClient(link);
        MS now = GetNow();
        if (now - link->sendAt >= KeepAliveMs) {
            Chunk *chunk = buildChunkCode(NULL, 0);
            terminalOutputAppend(link->terminal, chunk);
            link->sendAt = now;
        }
    }
}

