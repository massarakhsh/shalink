#include "terminal.h"
#include "interop.h"

int shaPoolInsert(ShaPool *pool, ShaChunk *chunk) {
    ShaChunk *pred = pool->lastChunk;
    ShaChunk *next = NULL;
    while (pred != NULL) {
        int cmp = shaCompare(pred->head.indexChunk, chunk->head.indexChunk);
        if (cmp == 0) return 0;
        if (cmp < 0) break;
        next = pred;
        pred = pred->predChunk;
    }
    if (pred != NULL) {
        chunk->predChunk = pred;
        pred->nextChunk = chunk;
    } else {
        pool->firstChunk = chunk;
    }
    if (next != NULL) {
        chunk->nextChunk = next;
        next->predChunk = chunk;
        if (chunk->createdAt > next->createdAt) chunk->createdAt = next->createdAt;
    } else {
        pool->lastChunk = chunk;
    }
    return 1;
}

void shaPoolAppend(ShaPool *pool, ShaChunk *chunk) {
    uint32_t indexChunk = pool->lastIndexChunk+1;
    pool->lastIndexChunk = indexChunk;
    chunk->head.indexChunk = indexChunk;
    shaPoolInsert(pool, chunk);
}

void shaPoolExtruct(ShaPool *pool, ShaChunk *chunk) {
    ShaChunk *pred = chunk->predChunk;
    ShaChunk *next = chunk->nextChunk;
    if (pred) {
        pred->nextChunk = next;
        chunk->predChunk = NULL;
    }
    else {
        pool->firstChunk = next;
    }
    if (next) {
        next->predChunk = pred;
        chunk->nextChunk = NULL;
    }
    else {
        pool->lastChunk = pred;
    }
}

uint32_t shaPoolFirst(ShaPool *pool) {
    return (pool->firstChunk != NULL) ? pool->firstChunk->head.indexChunk : 0;
}

uint32_t shaPoolCount(ShaPool *pool) {
    return (pool->lastChunk != NULL) ? pool->lastChunk->head.indexChunk-pool->firstChunk->head.indexChunk+1: 0;
}

void shaPoolClear(ShaPool *pool) {
    while (pool->firstChunk) {
        ShaChunk *chunk = pool->firstChunk;
        shaPoolExtruct(pool, chunk);
        shaChunkFree(chunk);
    }
}
