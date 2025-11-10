#include "terminal.h"
#include "interop.h"

ShaLink* shaBuildLink(ShaTerminal *terminal, const char *address, int port, int isServer) {
    ShaLink *link = (ShaLink*)calloc(1, sizeof(ShaLink));
    link->terminal = terminal;
    strncpy(link->address, address, sizeof(link->address));
    link->port = port;
    link->isServer = isServer;
    return link;
}

void shaLinkInsert(ShaTerminal *terminal, ShaLink *link) {
    link->predLink = terminal->lastLink;
    if (terminal->lastLink != NULL) terminal->lastLink->nextLink = link;
    else terminal->firstLink = link;
    terminal->lastLink = link;
    link->nextLink = NULL;
}

void shaLinkExtruct(ShaLink *link) {
    ShaTerminal *terminal = link->terminal;
    ShaLink *pred = link->predLink;
    ShaLink *next = link->nextLink;
    if (pred) {
        pred->nextLink = next;
        link->predLink = NULL;
    }
    else terminal->firstLink = next;
    if (next) {
        next->predLink = pred;
        link->nextLink = NULL;
    }
    else terminal->lastLink = pred;
}

void shaLinkReset(ShaLink *link) {
    while (link->firstGuest != NULL) {
        shaGuestExtruct(link, link->firstGuest);
    }
    if (link->socketId > 0) {
        //printf("Close socket\n");
        close(link->socketId);
        link->socketId = 0;
    }
    link->isBinded = 0;
    link->isOpened = 0;
}

void linkSync(ShaLink *link) {
    if (link->isServer) {
        for (ShaGuest *guest = link->firstGuest; guest != NULL; guest = guest->nextGuest) {
            if (GetNow() - guest->brief.lastSync >= guest->brief.rttAvg) {
                shaSyncOutput(link, guest);
                guest->brief.lastSync = GetNow();
            }
        }
    } else if (GetNow() - link->brief.lastSync >= link->brief.rttAvg) {
        shaSyncOutput(link, NULL);
        link->brief.lastSync = GetNow();
    }
}

void shaLinkStep(ShaLink *link) {
    if (!link->isOpened && GetSience(link->socketAt) >= ShaMSec * 100) {
        shaLinkOpen(link);
    }
    if (link->isOpened) {
        shaLinkInput(link);
        if (!link->terminal->isMirror) {
            shaLinkOutput(link);
            shaGuestControl(link);
            linkSync(link);
        }
    }
}

void shaLinkFree(ShaLink *link) {
    shaLinkReset(link);
    free(link);
}
