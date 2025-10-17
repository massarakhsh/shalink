#include "packet.h"
#include "interop.h"

void terminalPacketExtruct(Terminal *term, Packet *packet) {
    Packet *pred = packet->predPacket;
    Packet *next = packet->nextPacket;
    if (pred) {
        pred->nextPacket = next;
        packet->predPacket = NULL;
    }
    else term->firstPacket = next;
    if (next) {
        next->predPacket = pred;
        packet->nextPacket = NULL;
    }
    else term->lastPacket = pred;
}

void terminalPacketInsert(Terminal *term, Packet *packet) {
    Packet *pred = NULL;
    Packet *next = term->firstPacket;
    while (next != NULL) {
        if (packet->isDone && !next->isDone) break;
        if (packet->isDone != next->isDone) {
            if (next->channel > packet->channel) break;
            if (next->channel == packet->channel) {
                if (next->indexChannel > packet->indexChannel) break;
                if (next->indexChannel == packet->indexChannel) {
                    printf("Duplicate packet\n");
                    packetFree(packet);
                    return;
                }
            }
        }
        pred = next;
        next = next->nextPacket;
    }
    if (pred != NULL) {
        packet->predPacket = pred;
        pred->nextPacket = packet;
    } else {
        term->firstPacket = packet;
    }
    if (next != NULL) {
        packet->nextPacket = next;
        next->predPacket = packet;
    } else {
        term->lastPacket = packet;
    }
}

void terminalPacketStore(Terminal *term, Packet *packet) {
    if (packet->sizeData > 0) packet->data = (uint8_t*)calloc(1, packet->sizeData);
    for (Chunk *chunk = packet->firstChunk; chunk != NULL; chunk = chunk->nextChunk) {
        ChunkHead *hd = &chunk->head;
        if (packet->data != NULL && hd->sizeData > 0 && hd->offsetPacket+hd->sizeData <= packet->sizeData) {
            memcpy(packet->data + hd->offsetPacket, chunk->data, hd->sizeData);
        }
    }
    packetClearChunks(packet);
    terminalPacketExtruct(term, packet);
    packet->isDone = 1;
    terminalPacketInsert(term, packet);
    term->indexInput[packet->channel] = packet->indexChannel;
}

Packet* terminalFindPacket(Terminal *term, Chunk *chunk) {
    ChunkHead *hd = &chunk->head;
    Packet *pred = term->lastPacket;
    Packet *next = NULL;
    while (pred != NULL) {
        if (pred->isDone) break;
        if (pred->channel < hd->channel) break;
        if (pred->channel == hd->channel) {
            if (pred->indexChannel < hd->indexChannel) break;
            if (pred->indexChannel == hd->indexChannel) return pred;
        }
        next = pred;
        pred = pred->predPacket;
    }
    Packet *packet = buildPacket(hd->channel, hd->indexChannel, hd->sizePacket);
    if (packet == NULL) {
        printf("ERROR. No memory for packet\n");
        return NULL;
    }
    if (pred != NULL) {
        packet->predPacket = pred;
        pred->nextPacket = packet;
    } else {
        term->firstPacket = packet;
    }
    if (next != NULL) {
        packet->nextPacket = next;
        next->predPacket = packet;
    } else {
        term->lastPacket = packet;
    }
    return packet;
}

