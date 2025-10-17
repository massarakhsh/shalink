#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

Guest* findGuest(Link *link, struct sockaddr_in *addr) {
    for (int g = 0; g < link->guestCount; g++) {
        if (memcmp(addr, &link->guests[g].addr, sizeof(struct sockaddr_in)) == 0) {
            return link->guests+g;
        }
    }
    return NULL;
}

Packet* terminalInputPacket(Terminal *term) {
    Packet *packet = NULL;
    if (term->firstPacket != NULL && term->firstPacket->isDone) {
        packet = term->firstPacket;
        terminalPacketExtruct(term, packet);
    }
    return packet;
}

