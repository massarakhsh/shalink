#include "terminal.h"
#include "interop.h"

#define MaxInputPauseMs 30000

ShaGuest* shaBuildGuest(struct sockaddr_in *addr) {
    ShaGuest *guest = (ShaGuest*)calloc(1, sizeof(ShaGuest));
    memcpy(&guest->addr, addr, sizeof(struct sockaddr_in));
    guest->brief.lastSync = GetNow();
    // guest->brief.lastInput = guest->brief.lastSync;
    // guest->brief.lastOutput = guest->brief.lastSync;
    return guest;
}

void shaGuestInsert(ShaLink *link, ShaGuest *guest) {
    guest->predGuest = link->lastGuest;
    if (link->lastGuest != NULL) link->lastGuest->nextGuest = guest;
    else link->firstGuest = guest;
    link->lastGuest = guest;
    guest->nextGuest = NULL;
}

void shaGuestExtruct(ShaLink *link, ShaGuest *guest) {
    ShaGuest *pred = guest->predGuest;
    ShaGuest *next = guest->nextGuest;
    if (pred) {
        pred->nextGuest = next;
        guest->predGuest = NULL;
    }
    else link->firstGuest = next;
    if (next) {
        next->predGuest = pred;
        guest->nextGuest = NULL;
    }
    else link->lastGuest = pred;
}

ShaGuest* shaGuestFind(ShaLink *link, struct sockaddr_in *addr) {
    int count = 0;
    MCS now = GetNow();
    ShaGuest *guest = link->firstGuest;
    while (guest != NULL) {
        ShaGuest *nextGuest = guest->nextGuest;
        if (addr != NULL && memcmp(addr, &guest->addr, sizeof(struct sockaddr_in)) == 0) {
            guest->brief.lastInputMs = now;
            return guest;
        } else if (now - guest->brief.lastInputMs > MaxInputPauseMs) {
            printf("Unlink client\n");
            shaGuestExtruct(link, guest);
        }
        guest = nextGuest;
    }
    if (addr != NULL) {
        guest = shaBuildGuest(addr);
        shaGuestInsert(link, guest);
        printf("New client linked\n");
    }
    return guest;
}

void shaGuestFree(ShaGuest *guest) {
    free(guest);
}
