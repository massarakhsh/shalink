#include "terminal.h"
#include "interop.h"

int channelInsert(ShaChannel *channel, ShaFrame *packet) {
    ShaFrame *pred = channel->lastFrame;
    ShaFrame *next = NULL;
    while (pred != NULL) {
        int cmp = shaCompare(pred->indexPacket, packet->indexPacket);
        if (cmp == 0) return 0;
        if (cmp < 0) break;
        next = pred;
        pred = pred->predFrame;
    }
    if (pred != NULL) {
        packet->predFrame = pred;
        pred->nextFrame = packet;
    } else {
        channel->firstFrame = packet;
    }
    if (next != NULL) {
        packet->nextFrame = next;
        next->predFrame = packet;
        //if (packet->createdAt > next->createdAt) packet->createdAt = next->createdAt;
    } else {
        channel->lastFrame = packet;
    }
    return 1;
}

void channelExtruct(ShaChannel *channel, ShaFrame *packet) {
    ShaFrame *pred = packet->predFrame;
    ShaFrame *next = packet->nextFrame;
    if (pred) {
        pred->nextFrame = next;
        packet->predFrame = NULL;
    }
    else {
        channel->firstFrame = next;
    }
    if (next) {
        next->predFrame = pred;
        packet->nextFrame = NULL;
    }
    else {
        channel->lastFrame = pred;
    }
}

void shaChannelClear(ShaChannel *channel) {
    while (1) {
        ShaFrame *packet = channel->firstFrame;
        if (packet == NULL) break;
        channelExtruct(channel, packet);
        shaPacketFree(packet);
    }
}

