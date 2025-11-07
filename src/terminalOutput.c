#include "terminal.h"
#include "interop.h"

void shaOutputData(ShaTerminal *terminal, uint8_t channel, const void *data, uint32_t size) {
    uint32_t indexChannel = terminal->indexPacket[channel]+1;
    terminal->indexPacket[channel] = indexChannel;
    uint32_t firstChunk = 0;
    uint32_t countChung = 0;
    const uint8_t *bytes = (const uint8_t*)data;
    uint32_t offset = 0;
    uint32_t left = size;
    while (left > 0) {
        uint16_t pot = (left <= ChunkInfoSize) ? left : ChunkInfoSize;
        ShaChunk *chunk = shaChunkBuildData(channel, indexChannel, offset, size, bytes+offset, pot);
        shaPoolAppend(&terminal->outputPool, chunk);
        if (countChung == 0) firstChunk = chunk->head.indexChunk;
        countChung++;
        offset += pot;
        left -= pot;
        terminal->statistic.sendDataBytes += pot;
    }
    if (countChung > 0) {
        for (ShaLink *link = terminal->firstLink; link != NULL; link = link->nextLink) {
            shaLinkOutputQueue(link, firstChunk, countChung);
        }
    }
    terminal->statistic.sendDataPackets++;
    terminal->statistic.sendTotalPackets++;
}

void shaOutputStep(ShaTerminal *terminal) {
    MCS now = GetNow();
    while (1) {
        ShaChunk *chunk = terminal->outputPool.firstChunk;
        if (chunk == NULL) break;
        if (chunk->createdAt + terminal->ParmMaxLatency > now) break;
        shaPoolExtruct(&terminal->outputPool, chunk);
        shaChunkFree(chunk);
    }
}

