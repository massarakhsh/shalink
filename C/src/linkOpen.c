#include <sys/socket.h>
#include <netdb.h>
#include "terminal.h"
#include "interop.h"

int openNonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int openConnection(int sockfd) {
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

int nameToAddress(const char *name, struct in_addr *addr) {
    if (name == NULL || name[0] == 0) name = "localhost";
    struct hostent *host = gethostbyname(name);  
    if (host == NULL) return -1;
    for (int i = 0; i < 16; i++) {
        struct in_addr *inaddr = (struct in_addr *)host->h_addr_list[i];
        if (inaddr == NULL) break;
        if (addr != NULL) memcpy(addr, inaddr, sizeof(struct in_addr));
        return 1;
    }
    return 0;
}

int nameFromAddress(const struct in_addr *addr, char *name) {
    if (addr == NULL) return -1;
    char *toname = inet_ntoa(*addr);
    if (toname == NULL) return -1;
    if (name != NULL) strcpy(name, toname);
    return 1;
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
        if (bind(link->socketId, (struct sockaddr*)&address, sizeof(address)) < 0) {
            shaLinkReset(link);
            return;
        }
        if (openNonblocking(link->socketId) < 0) {
            shaLinkReset(link);
            return;
        }
        link->isOpened = 1;
        printf("Listening %s on %s:%d\n", link->terminal->name, link->address, link->port);
    } else {
        if (openNonblocking(link->socketId) < 0) {
            shaLinkReset(link);
            return;
        }
        if (nameToAddress(link->address, &(address.sin_addr)) < 0) {
            shaLinkReset(link);
            return;
        } else if (connect(link->socketId, (struct sockaddr*)&address, sizeof(address)) == 0) {
            printf("Connected %s to %s:%d\n", link->terminal->name, link->address, link->port);
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
        int status = openConnection(link->socketId);
        
        if (status == 1) {
            printf("Connection %s to %s:%d OK\n", link->terminal->name, link->address, link->port);
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