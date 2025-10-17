#include "chunk.h"
#include "interop.h"

Chunk* buildChunkData(uint8_t channel, uint32_t indexChunk, uint32_t indexChannel, uint32_t offsetPacket, uint32_t sizePacket, const void *data, uint16_t sizeData) {
    Chunk *chunk = (Chunk*)calloc(1, sizeof(Chunk));
    chunk->head.proto = ChunkProtoData;
    chunk->head.channel = channel;
    chunk->head.sizeData = sizeData;
    chunk->head.indexChunk = indexChunk;
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
    memcpy(chunk, code, size);
    return chunk;
}

void chunkFree(Chunk *chunk) {
    free(chunk);
}

int chunkSend(Chunk *chunk, int sockfd) {
    ssize_t sent = send(sockfd, chunk, ChunkHeadSize + chunk->head.sizeData, 0);
    
    if (sent == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            printf("Буфер заполнен, данные будут отправлены позже\n");
            return 0;
        } else {
            printf("Реальная ошибка\n");
            return -1;
        }
    }
    
    return sent;
}
