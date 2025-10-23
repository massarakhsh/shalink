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

// Store complete packet to pool packets, collect data and free chunks
void terminalPacketStore(Terminal *term, Packet *packet) {
    if (packet->sizeData > 0) packet->data = (uint8_t*)calloc(1, packet->sizeData);
    for (Chunk *chunk = packet->firstChunk; chunk != NULL; chunk = chunk->nextChunk) {
        ChunkHead *hd = &chunk->head;
        if (packet->data != NULL && hd->sizeData > 0 && hd->offsetPacket+hd->sizeData <= packet->sizeData) {
            memcpy(packet->data + hd->offsetPacket, chunk->data, hd->sizeData);
        }
    }
    packetClearChunks(packet);
    Packet *pred = packet->predPacket;
    if (pred != NULL && !pred->isDone) {
        //printf("Skiped packet: %d, %d/%d\n", pred->indexChannel, pred->sizeDone, pred->sizeData);
    }
    terminalPacketExtruct(term, packet);
    packet->isDone = 1;
    terminalPacketInsert(term, packet);
    term->indexInput[packet->channel] = packet->indexChannel;
}

// Find (and create) packet for append chunk
Packet* terminalFindPacket(Terminal *term, const ChunkHead *head) {
    Packet *pred = term->lastPacket;
    Packet *next = NULL;
    while (pred != NULL) {
        if (pred->isDone) break;
        if (pred->channel < head->channel) break;
        if (pred->channel == head->channel) {
            int cmp = ioCompareIndex(pred->indexChannel, head->indexChannel);
            if (cmp < 0) break;
            if (cmp == 0) return pred;
        }
        next = pred;
        pred = pred->predPacket;
    }
    Packet *packet = buildPacket(head->channel, head->indexChannel, head->sizePacket);
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

// Clear pool, drop old uncompleted packets
void terminalPacketClearOld(Terminal *term) {
    MS now = GetNow();
    Packet *packet = term->firstPacket;
    while (packet != NULL) {
        Packet *nextPacket = packet->nextPacket;
        if (!packet->isDone) {
            if (packet->createdAt+term->latency < now) {
                terminalPacketExtruct(term, packet);
                packetFree(packet);
            }
        }
        packet = nextPacket;
    }
}

