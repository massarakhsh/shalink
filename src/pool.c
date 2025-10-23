#include "terminal.h"
#include "interop.h"

int poolInsert(Pool *pool, Chunk *chunk) {
    Chunk *pred = pool->lastChunk;
    Chunk *next = NULL;
    while (pred != NULL) {
        int cmp = ioCompareIndex(pred->head.indexChunk, chunk->head.indexChunk);
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
    } else {
        pool->lastChunk = chunk;
    }
    return 1;
}

void poolAppend(Pool *pool, Chunk *chunk) {
    uint32_t indexChunk = pool->indexChunk+1;
    pool->indexChunk = indexChunk;
    chunk->head.indexChunk = indexChunk;
    poolInsert(pool, chunk);
}

void poolExtruct(Pool *pool, Chunk *chunk) {
    Chunk *pred = chunk->predChunk;
    Chunk *next = chunk->nextChunk;
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

void poolClear(Pool *pool) {
    while (pool->firstChunk) {
        Chunk *chunk = pool->firstChunk;
        poolExtruct(pool, chunk);
        chunkFree(chunk);
        //printf("Output chunk purged\n");
    }
}

