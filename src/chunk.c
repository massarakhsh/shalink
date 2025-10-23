#include "chunk.h"
#include "interop.h"

Chunk* buildChunkData(uint8_t channel, uint32_t indexChannel, uint32_t offsetPacket, uint32_t sizePacket, const void *data, uint16_t sizeData) {
    Chunk *chunk = (Chunk*)calloc(1, sizeof(Chunk));
    chunk->head.proto = ChunkProtoData;
    chunk->head.channel = channel;
    chunk->head.sizeData = sizeData;
    chunk->head.indexChannel = indexChannel;
    chunk->head.offsetPacket = offsetPacket;
    chunk->head.sizePacket = sizePacket;
    if (data != NULL && sizeData>0) memcpy(chunk->data, data, sizeData);
    chunk->createdAt = GetNow();
    return chunk;
}

Chunk* buildChunkCode(const void *code, uint32_t size) {
    if (size > sizeof(Chunk)) return NULL;
    Chunk *chunk = (Chunk*)calloc(1, sizeof(Chunk));
    if (code != NULL && size > 0) memcpy(chunk, code, size);
    chunk->createdAt = GetNow();
    return chunk;
}

void chunkFree(Chunk *chunk) {
    free(chunk);
}

uint32_t chunkRight(Chunk *chunk) {
    return chunk->head.offsetPacket + chunk->head.sizeData;
}
