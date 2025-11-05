#include "terminal.h"
#include "interop.h"

void shaQueueInsert(ShaQueue *queue, uint32_t firstChunk, uint32_t countChunk) {
    if (countChunk <= 0) return;
    int pos = 0;
    ShaPot *pot = queue->pots;
    while (pos < queue->countPots) {
        if (firstChunk <= pot->firstChunk + pot->countChunk) break;
        pos++;
        pot++;
    }
    if (pos >= queue->countPots) {
        if (pos >= MaxQueuePots) {
            printf("ERROR. Overflow queue space A\n");
            return;
        }
        pot->firstChunk = firstChunk;
        pot->countChunk = countChunk;
        queue->countPots++;
        return;
    }
    if (firstChunk+countChunk < pot->firstChunk) {
        if (queue->countPots >= MaxQueuePots) {
            printf("ERROR. Overflow queue space B\n");
            return;
        }
        for (int r = queue->countPots; r > pos; r--) {
            memcpy(queue->pots+r, queue->pots+r-1, sizeof(ShaPot));
        }
        pot->firstChunk = firstChunk;
        pot->countChunk = countChunk;
        queue->countPots++;
        return;
    }
    if (firstChunk < pot->firstChunk) {
        pot->countChunk += pot->firstChunk-firstChunk;
        pot->firstChunk = firstChunk;
    }
    if (firstChunk+countChunk > pot->firstChunk+pot->countChunk) {
        pot->countChunk = firstChunk+countChunk - pot->firstChunk;
    }
    ShaPot *next = queue->pots+pos+1;
    while (pos+1 < queue->countPots) {
        uint32_t rightChunk = pot->firstChunk+pot->countChunk;
        if (rightChunk < next->firstChunk) break;
        if (rightChunk < next->firstChunk + next->countChunk) {
            pot->countChunk = next->firstChunk + next->countChunk - pot->firstChunk;
        }
        if (pos+2 < queue->countPots) {
            memcpy(queue->pots+pos+1, queue->pots+pos+2, sizeof(ShaPot)*(queue->countPots-pos-2));
        }
        queue->countPots--;
    }
}

int shaQueuePresent(ShaQueue *queue) {
    return (queue->countPots > 0) ? 1 : 0;
}

uint32_t shaQueueGet(ShaQueue *queue) {
    if (queue->countPots <= 0) return 0;
    ShaPot *pot = queue->pots;
    uint32_t first = pot->firstChunk;
    if (pot->countChunk > 1) {
        pot->firstChunk++;
        pot->countChunk--;
    } else {
        if (queue->countPots > 1) {
            memcpy(queue->pots, queue->pots+1, sizeof(ShaPot)*(queue->countPots-1));
        }
        queue->countPots--;
    }
    return first;
}

