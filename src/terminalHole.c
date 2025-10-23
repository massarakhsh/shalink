#include "packet.h"
#include "interop.h"

void terminalHolePacket(Terminal *terminal, Packet *packet) {
    if (packet->holeSize > 0) {
        Chunk *chunk = buildChunkCode(NULL, 0);
        chunk->head.proto = ChunkProtoRepeat;
        chunk->head.channel = packet->channel;
        chunk->head.indexChannel = packet->indexChannel;
        chunk->head.offsetPacket = packet->holeOffset;
        chunk->head.sizePacket = packet->holeSize;
        chunk->createdAt = GetNow();
        printf("Output repeat %d [%d]\n", chunk->head.indexChannel, chunk->head.sizePacket);
        terminalChunkAppend(terminal, chunk);
    }
    packet->holeSize = 0;
}

void terminalHoleControl(Terminal *term, Packet *packet, Chunk *chunk) {
    uint32_t offsetPred = 0;
    if (chunk->predChunk != NULL) offsetPred = chunkRight(chunk->predChunk);
    if (offsetPred < chunk->head.offsetPacket) {
        if (term->stepPacket != NULL && term->stepPacket != packet) {
            terminalHolePacket(term, term->stepPacket);
        }
        term->stepPacket = packet;
        if (packet->holeSize == 0) {
            packet->holeOffset = offsetPred;
            packet->holeSize = chunk->head.offsetPacket - offsetPred;
        } else {
            packet->holeSize = chunk->head.offsetPacket - packet->holeOffset;
        }
    }
}

void terminalHoleOutput(Terminal *term, Chunk *chunk) {
    printf("Input repeat %d [%d]\n", chunk->head.indexChannel, chunk->head.sizePacket);   
}

