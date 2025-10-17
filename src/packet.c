#include "packet.h"
#include "interop.h"

Packet* buildPacket(uint8_t channel, uint32_t indexChannel, uint32_t sizeData) {
    Packet *packet = (Packet*)calloc(1, sizeof(Packet));
    packet->channel = channel;
    packet->indexChannel = indexChannel;
    packet->sizeData = sizeData;
    packet->createdAt = GetNow();
    return packet;
}

void packetClearChunks(Packet *packet) {
    Chunk *chunk = packet->firstChunk;
    while (chunk != NULL) {
        Chunk *nextChunk = chunk->nextChunk;
        chunkFree(chunk);
        chunk = nextChunk;
    }
    packet->firstChunk = NULL;
    packet->lastChunk = NULL;
}

void packetFree(Packet *packet) {
    packetClearChunks(packet);
    if (packet->data) free(packet->data);
    free(packet);
}
