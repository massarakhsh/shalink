#include "terminal.h"
#include "interop.h"

int channelInsert(ShaChannel *channel, ShaPacket *packet) {
    ShaPacket *pred = channel->lastPacket;
    ShaPacket *next = NULL;
    while (pred != NULL) {
        int cmp = shaCompare(pred->indexPacket, packet->indexPacket);
        if (cmp == 0) return 0;
        if (cmp < 0) break;
        next = pred;
        pred = pred->predPacket;
    }
    if (pred != NULL) {
        packet->predPacket = pred;
        pred->nextPacket = packet;
    } else {
        channel->firstPacket = packet;
    }
    if (next != NULL) {
        packet->nextPacket = next;
        next->predPacket = packet;
        //if (packet->createdAt > next->createdAt) packet->createdAt = next->createdAt;
    } else {
        channel->lastPacket = packet;
    }
    return 1;
}

void channelExtruct(ShaChannel *channel, ShaPacket *packet) {
    ShaPacket *pred = packet->predPacket;
    ShaPacket *next = packet->nextPacket;
    if (pred) {
        pred->nextPacket = next;
        packet->predPacket = NULL;
    }
    else {
        channel->firstPacket = next;
    }
    if (next) {
        next->predPacket = pred;
        packet->nextPacket = NULL;
    }
    else {
        channel->lastPacket = pred;
    }
}

void shaChannelClear(ShaChannel *channel) {
    while (1) {
        ShaPacket *packet = channel->firstPacket;
        if (packet == NULL) break;
        channelExtruct(channel, packet);
        shaPacketFree(packet);
    }
}

