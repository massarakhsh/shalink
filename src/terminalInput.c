#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

void terminalInputChunkData(Terminal *term, Chunk *chunk);
void terminalInputRepeat(Terminal *term, Chunk *chunk);

void terminalInputChunk(Terminal *term, Chunk *chunk) {
    if (chunk->head.proto == ChunkProtoData) {
        terminalInputChunkData(term, chunk);
    } else if (chunk->head.proto == ChunkProtoRepeat) {
        terminalHoleOutput(term, chunk);
    } else {
        chunkFree(chunk);
    }
}

// Receive input chunk for packeting
void terminalInputChunkData(Terminal *term, Chunk *chunk) {
    ChunkHead *head = &chunk->head;
    if (ioCompareIndex(head->indexChannel, term->indexInput[head->channel]) <= 0) { // Old chunk, packet was complete or drop
        chunkFree(chunk);
        return;
    }
    if (head->offsetPacket+head->sizeData > head->sizePacket) { // Wrong bounds of chunk
        printf("ERROR. Bound of chunk offset=%d, size=%d, packet=%d\n", head->offsetPacket, head->sizeData, head->sizePacket);
        chunkFree(chunk);
        return;
    }
    Packet *packet = terminalFindPacket(term, head);
    if (packet == NULL) {   // Packet not found
        chunkFree(chunk);
        return;
    }
    Chunk *pred = packet->lastChunk;
    Chunk *next = NULL;
    while (pred != NULL) {
        if (pred->head.offsetPacket < head->offsetPacket) {
            if (chunkRight(pred) > head->offsetPacket) {
                printf("ERROR. Interference-A of chunks\n");
                chunkFree(chunk);
                return;
            }
            break;
        }
        if (pred->head.offsetPacket == head->offsetPacket) {
            if (pred->head.sizeData != head->sizeData) {
                printf("ERROR. Interference-C of chunks\n");
            }
            printf("Duplicate chunk\n");
            chunkFree(chunk);
            return;
        }
        if (head->offsetPacket + head->sizeData > pred->head.offsetPacket) {
            printf("ERROR. Interference-B of chunks\n");
            chunkFree(chunk);
            return;
        }
        next = pred;
        pred = pred->predChunk;
    }
    if (pred != NULL) {
        chunk->predChunk = pred;
        pred->nextChunk = chunk;
    } else {
        packet->firstChunk = chunk;
    }
    if (next != NULL) {
        chunk->nextChunk = next;
        next->predChunk = chunk;
    } else {
        packet->lastChunk = chunk;
    }
    packet->sizeDone += head->sizeData;
    if (packet->appendedAt < chunk->createdAt) packet->appendedAt = chunk->createdAt;
    if (packet->sizeDone >= packet->sizeData) {
        terminalPacketStore(term, packet);
    } else {
        terminalHoleControl(term, packet, chunk);
    }
}

