#include "terminal.h"
#include "interop.h"

// Answer to chunk-request, repeat send chunks
void shaSyncInput(ShaLink *link, ShaGuest *guest, ShaChunk *chunk) {
    ShaTerminal *terminal = link->terminal;
    ShaSync *sync = (ShaSync*)chunk->data;
    ShaBrief *brief = (guest != NULL) ? &guest->brief : &link->brief;
    ShaQueue *queue = (guest != NULL) ? &guest->queue : &link->queue;
    brief->youSync = *sync;
    MCS now = GetNow();
    brief->meTime = now;
    brief->youTime =sync->meTime;
    MCS rtt = 0;
    if (sync->youTime != 0) {
        rtt = now - sync->youTime;
        rtt = (rtt > sync->mePause) ? rtt - sync->mePause : 0;
    }
    MCS rtts = 0;
    for (int b = 0; b < MaxRtts; b++) {
        MCS rt = (b+1 < MaxRtts) ? brief->rtts[b+1] : rtt;
        brief->rtts[b] = rt;
        rtts += rt;
    }
    brief->rttAvg = rtts / MaxRtts;
    link->terminal->statistic.recvSyncChunks++;
    link->terminal->statistic.meRTT = (guest != NULL) ? guest->brief.rttAvg : link->brief.rttAvg;
    link->terminal->statistic.youRTT = sync->meRTT;
    uint32_t myFirst = shaPoolFirst(&terminal->outputPool);
    uint32_t myCount = shaPoolCount(&terminal->outputPool);
    ShaSyncPair *pairs = (ShaSyncPair*)((uint8_t*)sync + sizeof(ShaSync));
    for (int count = 0; count < sync->countHoles; count++) {
        uint32_t index = pairs[count].needYouFirst;
        uint32_t size = pairs[count].needYouCount;
        //printf("%ld: Answer repeat %d [%d]\n", GetNow(), index, size);
        shaQueueInsert(queue, index, size);
    }
    shaChunkFree(chunk);
}

// Fix hole of input chunks to request-chunk and send to output
// void holeSend(ShaTerminal *terminal, uint32_t index, uint32_t size) {
//     ShaChunk *chunk = shaChunkBuildCode(NULL, 0);
//     chunk->head.proto = ChunkProtoRequest;
//     chunk->head.indexChunk = index;
//     chunk->head.sizePacket = size;
//     chunk->createdAt = GetNow();
//     //printf("Output repeat %d [%d]\n", chunk->head.indexChunk, chunk->head.sizePacket);
//     shaOutputAppend(terminal, chunk);
// }

void syncAddHole(ShaSync *sync, uint32_t index, uint32_t count) {
    ShaSyncPair *pairs = (ShaSyncPair*)((uint8_t*)sync + sizeof(ShaSync));
    pairs[sync->countHoles].needYouFirst = index;
    pairs[sync->countHoles].needYouCount = count;
    //printf("%ld: Request repeat %d [%d]\n", GetNow(), index, count);
    sync->countHoles++;
}

void syncFillHoles(ShaTerminal *terminal, ShaBrief *brief, ShaSync *sync) {
    MCS now = GetNow();
    uint32_t lastChunk = terminal->lastDoneChunk;
    for (ShaChunk *chunk = terminal->inputPool.firstChunk; chunk != NULL; chunk = chunk->nextChunk) {
        if (sync->countHoles >= MaxHolesSync) break;
        uint32_t need = lastChunk+1;
        uint32_t real = chunk->head.indexChunk;
        lastChunk = real;
        if (shaCompare(real, terminal->lastInputChunk) > 0) terminal->lastInputChunk = real;
        if (shaCompare(real, need) > 0) {
            syncAddHole(sync, need, real - need);
        }
    }
    if (brief->youSync.indexMyCount > 0) {
        uint32_t lastYou = brief->youSync.indexMyFirst+brief->youSync.indexMyCount-1;
        if (shaCompare(lastYou, lastChunk) > 0) {
            syncAddHole(sync, lastChunk+1, lastYou-lastChunk);
        }
    }
}

void shaSyncOutput(ShaLink *link, ShaGuest *guest) {
    ShaTerminal *terminal = link->terminal;
    ShaChunk *chunk = shaChunkBuildSync();
    ShaSync *sync = (ShaSync*)chunk->data;
    ShaBrief *brief = (guest != NULL) ? &guest->brief : &link->brief;
    MCS now = GetNow();
    sync->meTime = now;
    sync->mePause = (brief->meTime != 0) ? now - brief->meTime : 0;
    sync->youTime = brief->youTime, sync;
    sync->meRTT = (guest != NULL) ? guest->brief.rttAvg : link->brief.rttAvg;
    sync->indexYouRead = link->terminal->lastDoneChunk;
    sync->indexMyFirst = shaPoolFirst(&terminal->outputPool);
    sync->indexMyCount = shaPoolCount(&terminal->outputPool);
    syncFillHoles(link->terminal, brief, sync);
    chunk->head.sizeData = sizeof(ShaSync) + sizeof(ShaSyncPair) * sync->countHoles;
    shaLinkOutputGuest(link, guest, chunk);
}
