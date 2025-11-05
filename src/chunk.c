#include "chunk.h"
#include "interop.h"

#define idxLow 0x40000000
#define idxMed 0x80000000
#define idxHi 0xc0000000

int shaCompare(uint32_t a, uint32_t b) {
    if (a == b) return 0;
    uint32_t delta = a - b;
    if (delta <= idxLow) return 1;
    if (delta >= idxHi) return -1;
    return 2;
}

ShaChunk* shaChunkBuildData(uint8_t channel, uint32_t indexPacket, uint32_t offsetPacket, uint32_t sizePacket, const void *data, uint16_t sizeData) {
    ShaChunk *chunk = (ShaChunk*)calloc(1, sizeof(ShaChunk));
    chunk->head.proto = ChunkProtoData;
    chunk->head.channel = channel;
    chunk->head.sizeData = sizeData;
    chunk->head.indexPacket = indexPacket;
    chunk->head.offsetPacket = offsetPacket;
    chunk->head.sizePacket = sizePacket;
    if (data != NULL && sizeData>0) memcpy(chunk->data, data, sizeData);
    chunk->createdAt = GetNow();
    return chunk;
}

ShaChunk* shaChunkBuildCode(const void *code, uint32_t size) {
    if (size > sizeof(ShaChunk)) return NULL;
    ShaChunk *chunk = (ShaChunk*)calloc(1, sizeof(ShaChunk));
    if (code != NULL && size > 0) memcpy(chunk, code, size);
    chunk->createdAt = GetNow();
    return chunk;
}

ShaChunk* shaChunkBuildSync() {
    ShaChunk *chunk = (ShaChunk*)calloc(1, sizeof(ShaChunk));
    chunk->head.proto = ChunkProtoSync;
    chunk->head.sizeData = ChunkHeadSize + sizeof(ShaSync);
    chunk->createdAt = GetNow();
    return chunk;
}

void shaChunkFree(ShaChunk *chunk) {
    free(chunk);
}

uint32_t shaChunkRight(ShaChunk *chunk) {
    return chunk->head.offsetPacket + chunk->head.sizeData;
}
