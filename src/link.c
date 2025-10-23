#include "terminal.h"
#include "interop.h"

Link* buildLink(Terminal *term, const char *address, int port, int isServer) {
    Link *link = (Link*)calloc(1, sizeof(Link));
    link->terminal = term;
    strncpy(link->address, address, sizeof(link->address));
    link->port = port;
    link->isServer = isServer;
    return link;
}

void linkReset(Link *link) {
    if (link->socketId > 0) {
        printf("Close socket\n");
        close(link->socketId);
        link->socketId = 0;
    }
    while (link->guestCount>0) {
        link->guestCount--;
        memset(link->guests+link->guestCount, 0, sizeof(Guest));
    }
    link->isBinded = 0;
    link->isOpened = 0;
}

void linkStep(Link *link) {
    if (!link->isOpened) {
        if (!link->isBinded) linkOpenBind(link);
        if (link->isBinded && !link->isOpened) linkOpenConnect(link);
    }
    if (link->isOpened) {
        linkInput(link);
        linkOutput(link);
    }
}

Guest* linkFindGuest(Link *link, struct sockaddr_in *addr) {
    int count = 0;
    MS now = GetNow();
    Guest *free = NULL;
    for (int g = 0; g < MaxGuestCount; g++) {
        Guest *guest = link->guests+g;
        if (addr != NULL && memcmp(addr, &guest->addr, sizeof(struct sockaddr_in)) == 0) {
            guest->lastIn = now;
            return guest;
        } else if (guest->lastIn > 0 && now - guest->lastIn > 60000) {
            printf("Unlink client\n");
            memset(guest, 0, sizeof(Guest));
        }
        if (guest->lastIn > 0) {
            count++;
        } else if (addr != NULL && free == NULL) {
            free = guest;
        }
    }
    if (addr != NULL && free != NULL) {
        memcpy(&free->addr, addr, sizeof(struct sockaddr_in));
        free->lastIn = now;
        printf("New client linked\n");
        count--;
    }
    link->guestCount = count;
    return free;
}

