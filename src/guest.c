#include "terminal.h"
#include "interop.h"

#define MaxInputPauseMs 30*ShaSec

ShaGuest* shaBuildGuest(struct sockaddr_in *addr) {
    ShaGuest *guest = (ShaGuest*)calloc(1, sizeof(ShaGuest));
    memcpy(&guest->addr, addr, sizeof(struct sockaddr_in));
    guest->brief.lastSync = GetNow();
    char ip_str[INET_ADDRSTRLEN];
    if (inet_ntop(AF_INET, &(addr->sin_addr), ip_str, INET_ADDRSTRLEN) != NULL) {
        unsigned short port = ntohs(addr->sin_port);
        snprintf(guest->saddr, sizeof(guest->saddr), "%s:%d", ip_str, port);
    }
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
    ShaGuest *guest = link->firstGuest;
    while (guest != NULL) {
        ShaGuest *nextGuest = guest->nextGuest;
        if (addr != NULL && memcmp(addr, &guest->addr, sizeof(struct sockaddr_in)) == 0) {
            guest->brief.lastInputMs = GetNow();
            return guest;
        }
        guest = nextGuest;
    }
    if (addr != NULL) {
        guest = shaBuildGuest(addr);
        guest->brief.lastInputMs = GetNow();
        shaGuestInsert(link, guest);
        printf("New client linked: %s\n", guest->saddr);
    }
    return guest;
}

void shaGuestControl(ShaLink *link) {
    ShaGuest *guest = link->firstGuest;
    while (guest != NULL) {
        ShaGuest *nextGuest = guest->nextGuest;
        if (GetSience(guest->brief.lastInputMs) > MaxInputPauseMs) {
            printf("Unlink passive client: %s\n", guest->saddr);
            shaGuestExtruct(link, guest);
        }
        guest = nextGuest;
    }
}

void shaGuestFree(ShaGuest *guest) {
    free(guest);
}
