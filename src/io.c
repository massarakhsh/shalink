#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

int ioSetNonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

int ioProbeConnection(int sockfd) {
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
