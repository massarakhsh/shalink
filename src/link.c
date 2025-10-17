#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

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

void linkBind(Link *link) { 
    struct sockaddr_in address;

    linkReset(link);
    link->socketAt = GetNow();
    link->socketId = socket(AF_INET, SOCK_STREAM, 0);
    if (link->socketId <= 0) {
        printf("No get socket\n");
        return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(link->address);
    address.sin_port = htons(link->port);

    if (link->isServer) {
        if (bind(link->socketId, (struct sockaddr *)&address, sizeof(address)) < 0) {
            linkReset(link);
            return;
        }
        if (listen(link->socketId, 10) < 0) {
            linkReset(link);
            return;
        }
        if (ioSetNonblocking(link->socketId) < 0) {
            linkReset(link);
            return;
        }
        link->isOpened = 1;
        printf("Listening on %s:%d\n", link->address, link->port);
    } else {
        if (ioSetNonblocking(link->socketId) < 0) {
            linkReset(link);
            return;
        }
        
        if (connect(link->socketId, (struct sockaddr *)&address, sizeof(address)) == 0) {
            //printf("Connected to %s:%d\n", conn->address, conn->port);
            link->isOpened = 1;
        } else if (errno != EINPROGRESS) {
            linkReset(link);
            return;
        }
    }
    link->isBinded = 1;
}

void linkOpen(Link *link) {
    if (!link->isServer) {
        int status = ioProbeConnection(link->socketId);
        
        if (status == 1) {
            //printf("Connection OK\n");
            link->isOpened = 1;
        } else if (status < 0) {
            linkReset(link);
        }
    }
}

void linkStep(Link *link) {
    if (!link->isOpened) {
        if (!link->isBinded) linkBind(link);
        if (link->isBinded && !link->isOpened) linkOpen(link);
    }
    if (link->isOpened) {
        linkInput(link);
        outputLink(link);
    }
}

