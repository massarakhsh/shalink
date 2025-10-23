#include "terminal.h"
#include "interop.h"

#define idxLow 0x40000000
#define idxMed 0x80000000
#define idxHi 0xc0000000

int ioCompareIndex(uint32_t a, uint32_t b) {
    if (a == b) return 0;
    if (a < b && b < idxMed) return -1;
    if (a >= idxLow && a < b && b < idxHi) return -1;
    if (a >= idxMed && a < b) return -1;
    if (a >= idxHi && b < idxLow) return -1;
    return 1;
}

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
