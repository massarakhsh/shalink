#include "packet.h"
#include "interop.h"

ShaPacket* shaBuildPacket(uint8_t channel, uint32_t indexChannel, uint32_t sizeData) {
    ShaPacket *packet = (ShaPacket*)calloc(1, sizeof(ShaPacket));
    packet->channel = channel;
    packet->indexPacket = indexChannel;
    packet->sizeData = sizeData;
    packet->createdAt = GetNow();
    return packet;
}

void shaPacketExtruct(ShaTerminal *terminal, ShaPacket *packet) {
    ShaChannel *toChannel = terminal->inputChannel+packet->channel;
    ShaPacket *pred = packet->predPacket;
    ShaPacket *next = packet->nextPacket;
    if (pred) {
        pred->nextPacket = next;
        packet->predPacket = NULL;
    }
    else toChannel->firstPacket = next;
    if (next) {
        next->predPacket = pred;
        packet->nextPacket = NULL;
    }
    else toChannel->lastPacket = pred;
}

void shaPacketInsert(ShaTerminal *terminal, ShaPacket *packet) {
    ShaChannel *toChannel = terminal->inputChannel+packet->channel;
    ShaPacket *pred = NULL;
    ShaPacket *next = toChannel->firstPacket;
    while (next != NULL) {
        if (next->indexPacket > packet->indexPacket) break;
        if (next->indexPacket == packet->indexPacket) {
            printf("Duplicate packet\n");
            shaPacketFree(packet);
            return;
        }
        pred = next;
        next = next->nextPacket;
    }
    if (pred != NULL) {
        packet->predPacket = pred;
        pred->nextPacket = packet;
    } else {
        toChannel->firstPacket = packet;
    }
    if (next != NULL) {
        packet->nextPacket = next;
        next->predPacket = packet;
    } else {
        toChannel->lastPacket = packet;
    }
}

// Store complete packet to pool packets, collect data and free chunks
void shaPacketStore(ShaTerminal *terminal, ShaPacket *packet) {
    packet->sizeDone = 0;
    if (packet->sizeData > 0) {
        packet->data = (uint8_t*)calloc(1, packet->sizeData);
        if (packet->data == NULL) {
            printf("Packet %d data: no memory\n", packet->indexPacket);
            shaPacketExtruct(terminal, packet);
            shaPacketFree(packet);
            return;
        }
    }
    for (ShaChunk *chunk = terminal->inputPool.firstChunk; chunk != NULL; chunk = chunk->nextChunk) {
        ChunkHead *head = &chunk->head;
        if (head->channel == packet->channel && head->indexPacket == packet->indexPacket) {
            chunk->Used = 1;
            if (packet->data != NULL && head->sizeData > 0 && head->offsetPacket+head->sizeData <= packet->sizeData) {
                memcpy(packet->data + head->offsetPacket, chunk->data, head->sizeData);
                packet->sizeDone += head->sizeData;
            }
        }
    }
    if (packet->sizeDone >= packet->sizeData) {
        packet->storedAt = GetNow();
    } else {
        printf("%ld: Packet %d was breaked\n", GetNow(), packet->indexPacket);
        shaPacketExtruct(terminal, packet);
        shaPacketFree(packet);
    }
}

// Find (and create) packet for append chunk
ShaPacket* shaPacketFind(ShaTerminal *terminal, ShaChunk *chunk, int need) {
    ChunkHead *head = &chunk->head;
    if (head->channel >= MaxChannelCount) return NULL;
    ShaChannel *toChannel = terminal->inputChannel+head->channel;
    ShaPacket *pred = toChannel->lastPacket;
    ShaPacket *next = NULL;
    while (pred != NULL) {
        int cmp = shaCompare(pred->indexPacket, head->indexPacket);
        if (cmp < 0) break;
        if (cmp == 0) return pred;
        next = pred;
        pred = pred->predPacket;
    }
    if (!need) return NULL;

    ShaPacket *packet = shaBuildPacket(head->channel, head->indexPacket, head->sizePacket);
    if (packet == NULL) {
        printf("ERROR. No memory for packet\n");
        return NULL;
    }
    if (pred != NULL) {
        packet->predPacket = pred;
        pred->nextPacket = packet;
    } else {
        toChannel->firstPacket = packet;
    }
    if (next != NULL) {
        packet->nextPacket = next;
        next->predPacket = packet;
    } else {
        toChannel->lastPacket = packet;
    }
    terminal->statistic.recvTotalPackets++;
    return packet;
}

void shaPacketFree(ShaPacket *packet) {
    if (packet->data) free(packet->data);
    free(packet);
}
