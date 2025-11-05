#include "terminal.h"
#include "interop.h"

int shaSetNonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int shaProbeConnection(int sockfd) {
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLOUT | POLLERR;
    pfd.revents = 0;
    
    int result = poll(&pfd, 1, 0);
    
    if (result < 0) {
        return -1;
    }
    if (result == 0) {
        return 0;
    }
    if (pfd.revents & POLLERR) {
        return -1;
    }    
    if (pfd.revents & POLLOUT) {
        return 1;
    }
    
    return 0;
}

void shaLinkOpenBind(ShaLink *link) { 
    struct sockaddr_in address;

    shaLinkReset(link);
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
            shaLinkReset(link);
            return;
        }
        if (shaSetNonblocking(link->socketId) < 0) {
            shaLinkReset(link);
            return;
        }
        link->isOpened = 1;
        printf("Listening on %s:%d\n", link->address, link->port);
    } else {
        if (shaSetNonblocking(link->socketId) < 0) {
            shaLinkReset(link);
            return;
        }
        inet_pton(AF_INET, link->address, &address.sin_addr);
        if (connect(link->socketId, (struct sockaddr *)&address, sizeof(address)) == 0) {
            printf("Connected to %s:%d\n", link->address, link->port);
            link->isOpened = 1;
        } else if (errno != EINPROGRESS) {
            shaLinkReset(link);
            return;
        }
    }
    link->isBinded = 1;
}

void shaLinkOpenConnect(ShaLink *link) {
    if (!link->isServer) {
        int status = shaProbeConnection(link->socketId);
        
        if (status == 1) {
            printf("Connection to %s:%d OK\n", link->address, link->port);
            link->isOpened = 1;
        } else if (status < 0) {
            shaLinkReset(link);
        }
    }
}

void shaLinkOpen(ShaLink *link) {
    if (!link->isBinded) shaLinkOpenBind(link);
    if (link->isBinded && !link->isOpened) shaLinkOpenConnect(link);
}