#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

// Receive input chunk for packeting
void shaInputData(ShaTerminal *terminal, ShaChunk *chunk) {
    ChunkHead *head = &chunk->head;
    //printf("Input chunk %d\n", head->indexChunk);
    if (head->offsetPacket+head->sizeData > head->sizePacket) { // Wrong bounds of chunk
        printf("ERROR. Bound of chunk offset=%d, size=%d, packet=%d\n", head->offsetPacket, head->sizeData, head->sizePacket);
        shaChunkFree(chunk);
        return;
    }
    int cmp = shaCompare(head->indexChunk, terminal->lastDoneChunk);
    if (cmp <= 0) { // the chunk very old
        terminal->statistic.recvRepeatChunks++;
        shaChunkFree(chunk);
        return;
    } else if (cmp != 1) {    // reorder indexes
        printf("ERROR. Reorder input pool\n");
        shaPoolClear(&terminal->inputPool);
        terminal->lastDoneChunk = head->indexChunk;
        terminal->lastInputChunk = head->indexChunk;
    }
    if (!shaPoolInsert(&terminal->inputPool, chunk)) { // the chunk already received
        terminal->statistic.recvRepeatChunks++;
        shaChunkFree(chunk);
        return;
    }
    if (shaCompare(head->indexChunk, terminal->lastInputChunk) > 0) {
        terminal->lastInputChunk = head->indexChunk;
    }
    uint32_t nextDone = terminal->lastDoneChunk+1;
    for (ShaChunk *nextChunk = chunk; nextChunk != NULL; nextChunk = nextChunk->nextChunk) {
        if (nextDone != nextChunk->head.indexChunk) break;
        terminal->lastDoneChunk = nextDone;
        nextDone++;
    }

    ShaPacket *packet = shaPacketFind(terminal, chunk, 1);
    if (packet == NULL) {   // Packet not found
        printf("ERROR. Packet %d not found\n", chunk->head.indexPacket);
        shaPoolExtruct(&terminal->inputPool, chunk);
        shaChunkFree(chunk);
        return;
    }
    packet->sizeDone += head->sizeData;
    terminal->statistic.recvDataChunks++;
    terminal->statistic.recvDataBytes += head->sizeData;
    if (packet->appendAt < chunk->createdAt) packet->appendAt = chunk->createdAt;
    if (packet->sizeDone >= packet->sizeData) {
        shaPacketStore(terminal, packet);
    }
}

ShaPacket* shaInputGetScane(ShaTerminal *terminal, uint8_t channel, int fun) {
    uint8_t bestChannel = 0;
    MCS bestAge = 0;
    ShaPacket *bestPacket = NULL;
    int fromCha = (fun > 0) ? channel : 0;
    int toCha = (fun > 0) ? channel : MaxChannelCount-1;
    for (int cha = fromCha; cha <= toCha; cha++) {
        while (1) {
            ShaPacket *packet = terminal->inputChannel[cha].firstPacket;
            if (packet == NULL) break;
            MCS now = GetNow();
            if (!packet->storedAt && now - packet->createdAt > terminal->ParmMaxLatency*4) {
                printf("%ld: Purge unfilled packet %d\n", GetNow(), packet->indexPacket);
                shaPacketExtruct(terminal, packet);
                shaPacketFree(packet);
                continue;
            }
            if (packet->storedAt) {
                if (bestPacket == NULL || packet->createdAt < bestAge) {
                    bestChannel = cha;
                    bestAge = packet->createdAt;
                    bestPacket = packet;
                }
            }
            break;
        }
    }
    if (bestPacket == NULL || fun < 0) return NULL;
    shaPacketExtruct(terminal, bestPacket);
    terminal->statistic.recvDataPackets++;
    return bestPacket;
}

void shaChannelStep(ShaTerminal *term) {
    shaInputGetScane(term, 0, -1);
}

ShaPacket* shaInputGetPacket(ShaTerminal *terminal) {
    return shaInputGetScane(terminal, 0, 0);
}

ShaPacket* shaInputGetChannel(ShaTerminal *terminal, uint8_t channel) {
    return shaInputGetScane(terminal, channel, 1);
}

void shaInputStep(ShaTerminal *terminal) {
    MCS now = GetNow();
    while (1) {
        ShaChunk *chunk = terminal->inputPool.firstChunk;
        if (chunk == NULL) break;
        if (chunk->createdAt + terminal->ParmMaxLatency > now) break;
        if (!chunk->Used) {
            //printf("%ld: Purge unused chunk %d for packet %d\n", GetNow(), chunk->head.indexChunk, chunk->head.indexPacket);
        }
        shaPoolExtruct(&terminal->inputPool, chunk);
        shaChunkFree(chunk);
    }
}

