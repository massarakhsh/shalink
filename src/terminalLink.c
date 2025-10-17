#include "terminal.h"
#include "interop.h"

void terminalChunkInput(Terminal *term, Chunk *chunk) {
    if (chunk->head.proto == ChunkProtoData) {
        terminalChunkInputData(term, chunk);
    } else {
        chunkFree(chunk);
    }
}

void terminalChunkInputData(Terminal *term, Chunk *chunk) {
    ChunkHead *hd = &chunk->head;
    if (hd->indexChannel <= term->indexInput[hd->channel]) {
        printf("Old packet\n");
        chunkFree(chunk);
        return;
    }
    if (hd->offsetPacket+hd->sizeData > hd->sizePacket) {
        printf("ERROR. Bound of chunk offset=%d, size=%d, packet=%d\n", hd->offsetPacket, hd->sizeData, hd->sizePacket);
        chunkFree(chunk);
        return;
    }
    Packet *packet = terminalFindPacket(term, chunk);
    if (packet == NULL) {
        printf("ERROR. packet not found: index=%d\n", hd->indexChannel);
        chunkFree(chunk);
        return;
    }
    Chunk *pred = packet->lastChunk;
    Chunk *next = NULL;
    while (pred != NULL) {
        if (pred->head.offsetPacket < hd->offsetPacket) {
            if (pred->head.offsetPacket + pred->head.sizeData > hd->offsetPacket) {
                printf("ERROR. Interference-A of chunks\n");
                chunkFree(chunk);
                return;
            }
            break;
        }
        if (pred->head.offsetPacket == hd->offsetPacket) {
            if (pred->head.sizeData != hd->sizeData) {
                printf("ERROR. Interference-C of chunks\n");
            }
            printf("Duplicate chunk\n");
            chunkFree(chunk);
            return;
        }
        if (hd->offsetPacket + hd->sizeData > pred->head.offsetPacket) {
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
    packet->sizeDone += hd->sizeData;
    //printf("Received %d from %d\n", packet->sizeDone, packet->sizeData);
    if (packet->sizeDone >= packet->sizeData) {
        terminalPacketStore(term, packet);
    }
}

void terminalLinkInsert(Terminal *term, Link *link) {
    link->predLink = term->lastLink;
    if (term->lastLink != NULL) term->lastLink->nextLink = link;
    else term->firstLink = link;
    term->lastLink = link;
    link->nextLink = NULL;
}

void terminalLinkExtruct(Terminal *term, Link *link) {
    Link *pred = link->predLink;
    Link *next = link->nextLink;
    if (pred) {
        pred->nextLink = next;
        link->predLink = NULL;
    }
    else term->firstLink = next;
    if (next) {
        next->predLink = pred;
        link->nextLink = NULL;
    }
    else term->lastLink = pred;
}

void terminalLinkRoll(Terminal *term) {
    Link *first = term->firstLink;
    Link *last = term->lastLink;
    if (first == NULL || last == NULL || first == last) return;
    term->firstLink = first->nextLink;
    term->firstLink->predLink = NULL;
    last->nextLink = first;
    first->predLink = last;
    first->nextLink = NULL;
    term->lastLink = first;
}

