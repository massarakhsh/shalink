#include "terminal.h"
#include "interop.h"

ShaPot* queueBuildPot() {
    ShaPot *pot = (ShaPot*)calloc(1, sizeof(ShaPot));
    return pot;
}

void queueFreePot(ShaPot *pot) {
    return free(pot);
}

void queueInsertPot(ShaQueue *queue, ShaPot *pred, ShaPot *pot, ShaPot *next) {
    if (pred != NULL) {
        pred->nextPot = pot;
        pot->predPot = pred;
    } else queue->firstPot = pot;
    if (next != NULL) {
        next->predPot = pot;
        pot->nextPot = next;
    } else queue->lastPot = pot;
}

void queueExtructPot(ShaQueue *queue, ShaPot *pot) {
    if (queue->currentPot == pot) {
        queue->currentPot = queue->currentPot->nextPot;
    }
    ShaPot *pred = pot->predPot;
    ShaPot *next = pot->nextPot;
    if (pred) {
        pred->nextPot = next;
        pot->predPot = NULL;
    }
    else queue->firstPot = next;
    if (next) {
        next->predPot = pred;
        pot->nextPot = NULL;
    }
    else queue->lastPot = pred;
}

void shaQueueInsert(ShaQueue *queue, uint32_t firstChunk, uint32_t countChunk) {
    if (countChunk <= 0) return;
    uint32_t rightChunk = firstChunk+countChunk;
    ShaPot *pred = queue->lastPot;
    ShaPot *next = NULL;
    while (pred != NULL) {
        if (shaCompare(rightChunk, pred->firstChunk) >= 0) break;
        next = pred;
        pred = pred->predPot;
    }
    if (pred == NULL || shaCompare(firstChunk, pred->firstChunk+pred->countChunk) > 0) {
        ShaPot *pot = queueBuildPot();
        pot->firstChunk = firstChunk;
        pot->countChunk = countChunk;
        queueInsertPot(queue, pred, pot, next);
        return;
    }
    if (shaCompare(rightChunk, pred->firstChunk+pred->countChunk) > 0) {
        pred->countChunk = rightChunk - pred->firstChunk;
    }
    if (shaCompare(firstChunk, pred->firstChunk) < 0) {
        next = pred;
        pred = pred->predPot;
        next->countChunk = next->firstChunk+next->countChunk-firstChunk;
        next->firstChunk = firstChunk;
        while (pred != NULL && shaCompare(pred->firstChunk+pred->countChunk, next->firstChunk) >= 0) {
            next->countChunk = next->firstChunk+next->countChunk-pred->firstChunk;
            next->firstChunk = pred->firstChunk;
            queueExtructPot(queue, pred);
            pred = next->predPot;
        }
    }
}

int shaQueuePresent(ShaQueue *queue) {
    return (queue->firstPot != NULL && queue->firstPot->countChunk > 0) ? 1 : 0;
}

uint32_t shaQueueGet(ShaQueue *queue) {
    ShaPot *pot = queue->currentPot;
    if (pot == NULL) {
        pot = queue->firstPot;
        queue->currentPot = pot;
    }
    if (pot == NULL) return 0;
    uint32_t firstChunk = pot->firstChunk;
    if (pot->countChunk > 1) {
        pot->firstChunk++;
        pot->countChunk--;
    } else {
        queue->currentPot = pot->nextPot;
        queueExtructPot(queue, pot);
        queueFreePot(pot);
    }
    return firstChunk;
}

