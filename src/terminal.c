#include "terminal.h"
#include "interop.h"

void terminalRun(Terminal *terminal);

void* terminalThread(void* it) {
    Terminal *terminal = (Terminal*)it;
    terminalRun(terminal);
    return NULL;
} 

Terminal* BuildTerminal(const char *name) {
    Terminal *terminal = (Terminal*)calloc(1, sizeof(Terminal));
    terminal->latency = (MS)200;
    strncpy(terminal->name, name, sizeof(terminal->name));
    pthread_mutex_init(&terminal->mutex, NULL);
    pthread_create(&terminal->hThread, NULL, terminalThread, terminal);
    return terminal;
}

Link* TerminalAddLink(Terminal *terminal, const char *address, int port, int isServer) {
    Link *link = buildLink(terminal, address, port, isServer);
    terminalLock(terminal);
    terminalLinkInsert(terminal, link);
    terminalUnlock(terminal);
    return link;
}

void TerminalSend(Terminal *terminal, uint8_t channel, const void *data, uint16_t size) {
    terminalLock(terminal);
    terminalSend(terminal, channel, data, size);
    terminalUnlock(terminal);
}

Packet* TerminalGet(Terminal *terminal) {
    terminalLock(terminal);
    Packet *packet = NULL;
    if (terminal->firstPacket != NULL && terminal->firstPacket->isDone) {
        packet = terminal->firstPacket;
        terminalPacketExtruct(terminal, packet);
    }
    terminalUnlock(terminal);
    return packet;
}

void TerminalStop(Terminal *terminal) {
    terminal->isStoping = 1;
}

void TerminalFree(Terminal *terminal) {
    terminal->isStoping = 1;
    while (!terminal->isStoped) {
        usleep(1000);
    }
    pthread_mutex_destroy(&terminal->mutex);
    free(terminal);
}

void PacketFree(Packet *packet) {
    packetFree(packet);
}


void terminalLock(Terminal *terminal) {
    pthread_mutex_lock(&terminal->mutex);
}

void terminalUnlock(Terminal *terminal) {
    pthread_mutex_unlock(&terminal->mutex);
}

void terminalStep(Terminal *terminal) {
    terminal->stepAt = GetNow();
    terminal->stepPauseMcs = 100;
    terminal->stepPacket = NULL;
    terminalLock(terminal);
    Link *link = terminal->firstLink;
    if (link != NULL) {
        linkStep(link);
    }
    terminalLinkRoll(terminal);
    terminalChunkClear(terminal);
    terminalPacketClear(terminal);
    if (terminal->stepPacket != NULL) terminalHolePacket(terminal, terminal->stepPacket);
    terminalUnlock(terminal);
    int pause = (terminal->stepPauseMcs > 0) ? terminal->stepPauseMcs : 1;
    usleep(pause);
}

void terminalRun(Terminal *terminal) {
    while (!terminal->isStoping) {
        terminalStep(terminal);
    }
    terminal->isStoped = 1;
}
