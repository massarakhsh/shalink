#include "pack.h"
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

ShaPack* shaChunkBuildData(uint8_t dBell, uint8_t channel, uint32_t indexPacket, uint32_t offsetPacket, uint32_t sizePacket, const void *data, uint16_t sizeData) {
    ShaPack *pack = (ShaPack*)calloc(1, sizeof(ShaPack));
    pack->head.dBell = dBell;
    pack->head.channel = channel;
    pack->head.sizeData = sizeData;
    pack->head.indexPacket = indexPacket;
    pack->head.offsetPacket = offsetPacket;
    pack->head.sizePacket = sizePacket;
    if (data != NULL && sizeData>0) memcpy(pack->data, data, sizeData);
    pack->createdAt = GetNow();
    pack->liveTo = pack->createdAt + DbToMCS(dBell);
    return pack;
}

ShaPack* shaChunkBuildCode(const void *code, uint32_t size) {
    if (size > sizeof(ShaPack)) return NULL;
    ShaPack *pack = (ShaPack*)calloc(1, sizeof(ShaPack));
    if (code != NULL && size > 0) memcpy(pack, code, size);
    pack->createdAt = GetNow();
    pack->liveTo = pack->createdAt + DbToMCS(pack->head.dBell);
    return pack;
}

ShaPack* shaChunkBuildSync() {
    ShaPack *pack = (ShaPack*)calloc(1, sizeof(ShaPack));
    pack->head.isSync = 1;
    pack->head.sizeData = PackHeadSize + sizeof(ShaSync);
    pack->createdAt = GetNow();
    pack->liveTo = pack->createdAt + DbToMCS(pack->head.dBell);
    return pack;
}

void shaChunkFree(ShaPack *pack) {
    free(pack);
}

uint32_t shaChunkRight(ShaPack *pack) {
    return pack->head.offsetPacket + pack->head.sizeData;
}
