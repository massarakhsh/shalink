#include "terminal.h"
#include "interop.h"

void terminalChunkInsert(Terminal *terminal, Chunk *chunk) {
    chunk->predChunk = terminal->lastChunk;
    if (terminal->lastChunk != NULL) terminal->lastChunk->nextChunk = chunk;
    else terminal->firstChunk = chunk;
    terminal->lastChunk = chunk;
    chunk->nextChunk = NULL;
    chunk->createdAt = GetNow();
}

void terminalChunkExtruct(Terminal *terminal, Chunk *chunk) {
    Chunk *pred = chunk->predChunk;
    Chunk *next = chunk->nextChunk;
    if (pred) {
        pred->nextChunk = next;
        chunk->predChunk = NULL;
    }
    else terminal->firstChunk = next;
    if (next) {
        next->predChunk = pred;
        chunk->nextChunk = NULL;
    }
    else terminal->lastChunk = pred;
}

void terminalChunkClear(Terminal *term) {
    MS now = GetNow();
    while (term->firstChunk != NULL && term->firstChunk->createdAt+term->latency < now) {
        Chunk *chunk = term->firstChunk;
        terminalChunkExtruct(term, chunk);
        chunkFree(chunk);
        //printf("Output chunk purged\n");
    }
}

void terminalPacketClear(Terminal *term) {
    MS now = GetNow();
    Packet *packet = term->firstPacket;
    while (packet != NULL) {
        Packet *nextPacket = packet->nextPacket;
        if (!packet->isDone) {
            if (packet->createdAt+term->latency < now) {
                packetFree(packet);
            }
        }
        packet = nextPacket;
    }
}

void terminalSend(Terminal *terminal, uint8_t channel, const void *data, uint16_t size) {
    uint32_t indexChannel = terminal->indexOutput[channel]+1;
    terminal->indexOutput[channel] = indexChannel;
    printf("TerminalSend [%d] index=%d\n", size, indexChannel);
    const uint8_t *bytes = (const uint8_t*)data;
    int offset = 0;
    int left = size;
    while (left > 0) {
        int pot = (left <= MaxChunkSize) ? left : MaxChunkSize;
        uint32_t indexChunk = terminal->indexChunk+1;
        terminal->indexChunk = indexChunk;
        Chunk *chunk = buildChunkData(channel, indexChunk, indexChannel, offset, size, bytes+offset, pot);
        offset += pot;
        left -= pot;
        terminalChunkInsert(terminal, chunk);
    }
}

