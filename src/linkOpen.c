#include "terminal.h"
#include "interop.h"

void linkOpenBind(Link *link) { 
    struct sockaddr_in address;

    linkReset(link);
    link->socketAt = GetNow();
    link->socketId = socket(AF_INET, SOCK_DGRAM, 0);
    if (link->socketId <= 0) {
        printf("No get socket\n");
        return;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(link->port);

    if (link->isServer) {
        if (bind(link->socketId, (struct sockaddr *)&address, sizeof(address)) < 0) {
            linkReset(link);
            return;
        }
        // if (listen(link->socketId, 10) < 0) {
        //     linkReset(link);
        //     return;
        // }
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
        inet_pton(AF_INET, link->address, &address.sin_addr);
        if (connect(link->socketId, (struct sockaddr *)&address, sizeof(address)) == 0) {
            printf("Connected to %s:%d\n", link->address, link->port);
            link->isOpened = 1;
        } else if (errno != EINPROGRESS) {
            linkReset(link);
            return;
        }
    }
    link->isBinded = 1;
    link->terminal->stepPauseMcs = 0;
}

void linkOpenConnect(Link *link) {
    if (!link->isServer) {
        int status = ioProbeConnection(link->socketId);
        
        if (status == 1) {
            printf("Connection OK\n");
            link->isOpened = 1;
            link->terminal->stepPauseMcs = 0;
        } else if (status < 0) {
            linkReset(link);
        }
    }
}

