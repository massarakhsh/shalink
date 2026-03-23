#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

void inputScanDone(ShaTerminal *terminal, ShaPack *chunk) {
    uint32_t nextDone = terminal->lastDoneChunk+1;
    for (ShaPack *nextChunk = chunk; nextChunk != NULL; nextChunk = nextChunk->nextChunk) {
        if (nextDone != nextChunk->head.indexChunk) break;
        terminal->lastDoneChunk = nextDone;
        nextDone++;
    }
}

// Receive input chunk for packeting
void shaInputData(ShaTerminal *terminal, ShaPack *chunk) {
    PackHead *head = &chunk->head;
    //printf("Input chunk %d\n", head->indexChunk);
    if (head->offsetPacket+head->sizeData > head->sizePacket) { // Wrong bounds of chunk
        printf("ERROR. Bound of chunk offset=%d, size=%d, packet=%d\n", head->offsetPacket, head->sizeData, head->sizePacket);
        shaChunkFree(chunk);
        return;
    }
    int cmp = shaCompare(head->indexChunk, terminal->lastDoneChunk);
    if (cmp <= 0) { // the chunk very old
        terminal->statistic.packetsLate++;
        shaChunkFree(chunk);
        return;
    } else if (cmp == 2) {    // reorder indexes
        printf("ERROR. Reorder input pool\n");
        shaPoolClear(&terminal->inputPool);
        terminal->lastDoneChunk = head->indexChunk;
        terminal->lastInputChunk = head->indexChunk;
    }
    if (!shaPoolInsert(&terminal->inputPool, chunk)) { // the chunk already received
        terminal->statistic.packetsDouble++;
        shaChunkFree(chunk);
        return;
    }
    if (shaCompare(head->indexChunk, terminal->lastInputChunk) > 0) {
        terminal->lastInputChunk = head->indexChunk;
    }
    inputScanDone(terminal, chunk);

    ShaFrame *packet = shaPacketFind(terminal, chunk, 1);
    if (packet == NULL) {   // Packet not found
        printf("ERROR. Packet %d not found\n", chunk->head.indexPacket);
        shaPoolExtruct(&terminal->inputPool, chunk);
        shaChunkFree(chunk);
        return;
    }
    packet->sizeDone += head->sizeData;
    terminal->statistic.packetsIn++;
    terminal->statistic.bytesIn += head->sizeData;
    if (packet->appendAt < chunk->createdAt) packet->appendAt = chunk->createdAt;
    if (packet->sizeDone >= packet->sizeData) {
        shaPacketStore(terminal, packet);
    }
}

void shaInputCountrol(ShaTerminal *terminal) {
    MCS now = GetNow();
    while (1) {
        ShaPack *chunk = terminal->inputPool.firstChunk;
        if (chunk == NULL) break;
        if (now < chunk->liveTo) break;
        if (!chunk->Used) {
            //printf("%ld: Purge unused chunk %d for packet %d\n", GetNow(), chunk->head.indexChunk, chunk->head.indexPacket);
        }
        if (shaCompare(chunk->head.indexChunk, terminal->lastDoneChunk) > 0) {
            terminal->lastDoneChunk = chunk->head.indexChunk;
        }
        terminal->statistic.framesLost++;
        shaPoolExtruct(&terminal->inputPool, chunk);
        shaChunkFree(chunk);
    }
    if (terminal->inputPool.firstChunk != NULL) {
        inputScanDone(terminal, terminal->inputPool.firstChunk);
    }
}

