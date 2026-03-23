#include "frame.h"
#include "interop.h"

ShaFrame* shaBuildFrame(uint8_t channel, uint32_t indexChannel, uint32_t sizeData) {
    ShaFrame *frame = (ShaFrame*)calloc(1, sizeof(ShaFrame));
    frame->channel = channel;
    frame->indexPacket = indexChannel;
    frame->sizeData = sizeData;
    frame->createdAt = GetNow();
    return frame;
}

void shaPacketExtruct(ShaTerminal *terminal, ShaFrame *frame) {
    ShaChannel *toChannel = terminal->inputChannel+frame->channel;
    ShaFrame *pred = frame->predFrame;
    ShaFrame *next = frame->nextFrame;
    if (pred) {
        pred->nextFrame = next;
        frame->predFrame = NULL;
    }
    else toChannel->firstFrame = next;
    if (next) {
        next->predFrame = pred;
        frame->nextFrame = NULL;
    }
    else toChannel->lastFrame = pred;
}

void shaPacketInsert(ShaTerminal *terminal, ShaFrame *frame) {
    ShaChannel *toChannel = terminal->inputChannel+frame->channel;
    ShaFrame *pred = NULL;
    ShaFrame *next = toChannel->firstFrame;
    while (next != NULL) {
        if (next->indexPacket > frame->indexPacket) break;
        if (next->indexPacket == frame->indexPacket) {
            printf("Duplicate frame\n");
            shaPacketFree(frame);
            return;
        }
        pred = next;
        next = next->nextFrame;
    }
    if (pred != NULL) {
        frame->predFrame = pred;
        pred->nextFrame = frame;
    } else {
        toChannel->firstFrame = frame;
    }
    if (next != NULL) {
        frame->nextFrame = next;
        next->predFrame = frame;
    } else {
        toChannel->lastFrame = frame;
    }
}

// Store complete frame to pool packets, collect data and free chunks
void shaPacketStore(ShaTerminal *terminal, ShaFrame *frame) {
    frame->sizeDone = 0;
    if (frame->sizeData > 0) {
        frame->data = (uint8_t*)calloc(1, frame->sizeData);
        if (frame->data == NULL) {
            printf("Packet %d data: no memory\n", frame->indexPacket);
            shaPacketExtruct(terminal, frame);
            shaPacketFree(frame);
            return;
        }
    }
    for (ShaPack *chunk = terminal->inputPool.firstChunk; chunk != NULL; chunk = chunk->nextChunk) {
        PackHead *head = &chunk->head;
        if (head->channel == frame->channel && head->indexPacket == frame->indexPacket) {
            chunk->Used = 1;
            if (frame->data != NULL && head->sizeData > 0 && head->offsetPacket+head->sizeData <= frame->sizeData) {
                memcpy(frame->data + head->offsetPacket, chunk->data, head->sizeData);
                frame->sizeDone += head->sizeData;
            }
        }
    }
    if (frame->sizeDone >= frame->sizeData) {
        frame->storedAt = GetNow();
    } else {
        //printf("%ld: Packet %d was breaked\n", GetNow(), frame->indexPacket);
        terminal->statistic.framesBreak++;
        shaPacketExtruct(terminal, frame);
        shaPacketFree(frame);
    }
}

// Find (and create) frame for append chunk
ShaFrame* shaPacketFind(ShaTerminal *terminal, ShaPack *chunk, int need) {
    PackHead *head = &chunk->head;
    if (head->channel >= MaxChannelCount) return NULL;
    ShaChannel *toChannel = terminal->inputChannel+head->channel;
    ShaFrame *pred = toChannel->lastFrame;
    ShaFrame *next = NULL;
    while (pred != NULL) {
        int cmp = shaCompare(pred->indexPacket, head->indexPacket);
        if (cmp < 0) break;
        if (cmp == 0) return pred;
        next = pred;
        pred = pred->predFrame;
    }
    if (!need) return NULL;

    ShaFrame *frame = shaBuildFrame(head->channel, head->indexPacket, head->sizePacket);
    if (frame == NULL) {
        printf("ERROR. No memory for frame\n");
        return NULL;
    }
    frame->liveAt = frame->createdAt + DbToMCS(head->dBell);
    if (pred != NULL) {
        frame->predFrame = pred;
        pred->nextFrame = frame;
    } else {
        toChannel->firstFrame = frame;
    }
    if (next != NULL) {
        frame->nextFrame = next;
        next->predFrame = frame;
    } else {
        toChannel->lastFrame = frame;
    }
    return frame;
}

void shaPacketFree(ShaFrame *frame) {
    if (frame->data) free(frame->data);
    free(frame);
}
