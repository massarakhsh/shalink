#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

void runOutputGuest(Link *link, Guest *guest) {
    Chunk *chunk = link->terminal->firstChunk;
    while (chunk != NULL) {
        uint32_t chi = chunk->head.indexChunk;
        if (BEFORE(guest->indexChunk, chi)) {
            if (chunkSend(chunk, guest->socketId) < 0) break;
            guest->indexChunk = chi;
        }
        chunk = chunk->nextChunk;
    }
}

void runOutputClient(Link *link) {
    Chunk *chunk = link->terminal->firstChunk;
    while (chunk != NULL) {
        uint32_t chi = chunk->head.indexChunk;
        if (BEFORE(link->indexChunk, chi)) {
            if (chunkSend(chunk, link->socketId) < 0) break;
            link->indexChunk = chi;
        }
        chunk = chunk->nextChunk;
    }
}

void outputLink(Link *link) {

    if (link->isServer) {
        for (int g = 0; g < link->guestCount; g++) {
            runOutputGuest(link, link->guests+g);
        }
    } else {
        runOutputClient(link);
    }
}

