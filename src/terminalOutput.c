#include "terminal.h"
#include "interop.h"

void terminalOutputAppend(Terminal *terminal, Chunk *chunk) {
    poolAppend(&terminal->outputPool, chunk);
}

void terminalOutputClear(Terminal *terminal) {
    MS now = GetNow();
    while (terminal->outputPool.firstChunk != NULL && terminal->outputPool.firstChunk->createdAt+terminal->latency < now) {
        Chunk *chunk = terminal->outputPool.firstChunk;
        poolExtruct(&terminal->outputPool, chunk);
        chunkFree(chunk);
        //printf("Output chunk purged\n");
    }
}

void terminalSend(Terminal *terminal, uint8_t channel, const void *data, uint16_t size) {
    uint32_t indexChannel = terminal->indexOutput[channel]+1;
    terminal->indexOutput[channel] = indexChannel;
    //printf("TerminalSend [%d] index=%d\n", size, indexChannel);
    const uint8_t *bytes = (const uint8_t*)data;
    int offset = 0;
    int left = size;
    while (left > 0) {
        int pot = (left <= ChunkInfoSize) ? left : ChunkInfoSize;
        Chunk *chunk = buildChunkData(channel, indexChannel, offset, size, bytes+offset, pot);
        offset += pot;
        left -= pot;
        terminalOutputAppend(terminal, chunk);
    }
}

