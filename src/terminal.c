#include "terminal.h"
#include "interop.h"

void terminalRun(ShaTerminal *terminal);
void terminalFree(ShaTerminal *terminal);

void* terminalThread(void* it) {
    ShaTerminal *terminal = (ShaTerminal*)it;
    terminalRun(terminal);
    return NULL;
} 

ShaTerminal* BuildTerminal(const char *name) {
    ShaTerminal *terminal = (ShaTerminal*)calloc(1, sizeof(ShaTerminal));
    strncpy(terminal->name, name, sizeof(terminal->name));
    terminal->ParmMaxLatency = ShaMSec*200;
    terminal->ParmMaxStorage = ShaSec*5;
    pthread_mutex_init(&terminal->mutex, NULL);
    pthread_create(&terminal->hThread, NULL, terminalThread, terminal);
    return terminal;
}

ShaLink* TerminalAddLink(ShaTerminal *terminal, const char *address, int port, int isServer) {
    ShaLink *link = shaBuildLink(terminal, address, port, isServer);
    shaLock(terminal);
    shaLinkInsert(terminal, link);
    shaUnlock(terminal);
    return link;
}

void TerminalSend(ShaTerminal *terminal, uint8_t channel, const void *data, uint32_t size) {
    shaLock(terminal);
    shaOutputData(terminal, channel, data, size);
    shaUnlock(terminal);
}

ShaPacket* TerminalGetPacket(ShaTerminal *terminal) {
    shaLock(terminal);
    ShaPacket *packet = shaInputGetPacket(terminal);
    shaUnlock(terminal);
    return packet;
}

ShaPacket* TerminalGetChannel(ShaTerminal *terminal, uint8_t channel) {
    shaLock(terminal);
    ShaPacket *packet = shaInputGetChannel(terminal, channel);
    shaUnlock(terminal);
    return packet;
}

int TerminalStop(ShaTerminal *terminal) {
    terminal->isStoping = 1;
    for (int ms = 0; ms < 3000; ms++) {
        if (terminal->isStoped) return 1;
        Sleep(ShaMSec);
    }
    return 0;
}

int TerminalIsReady(ShaTerminal *terminal) {
    int ready = 0;
    shaLock(terminal);
    for (ShaLink *link = terminal->firstLink; link != NULL; link = link->nextLink) {
        if (link->isOpened) {
            if (!link->isServer) {
                ready = 1;
                break;
            } else if (link->firstGuest != NULL) {
                ready = 1;
                break;
            }
        }
    }    
    shaUnlock(terminal);
    return ready;
}

int TerminalFree(ShaTerminal *terminal) {
    if (!TerminalStop(terminal)) return 0;
    shaLock(terminal);
    terminalFree(terminal);
    shaUnlock(terminal);
    pthread_mutex_destroy(&terminal->mutex);
    free(terminal);
    return 1;
}

void PacketFree(ShaPacket *packet) {
    shaPacketFree(packet);
}

void shaLock(ShaTerminal *terminal) {
    pthread_mutex_lock(&terminal->mutex);
}

void shaUnlock(ShaTerminal *terminal) {
    pthread_mutex_unlock(&terminal->mutex);
}

void shaStep(ShaTerminal *terminal) {
    shaLock(terminal);
    terminal->stepPause = 100*ShaMCSec;
    for (ShaLink *link = terminal->firstLink; link != NULL; link = link->nextLink) {
        shaLinkStep(link);
    }
    if (!terminal->isMirror) {
        shaInputCountrol(terminal);
        shaOutputControl(terminal);
        shaChannelStep(terminal);
    }
    shaUnlock(terminal);
    if (terminal->stepPause > 0) Sleep(terminal->stepPause);
}

void terminalRun(ShaTerminal *terminal) {
    while (!terminal->isStoping) {
        shaStep(terminal);
    }
    terminal->isStoped = 1;
}

void terminalFree(ShaTerminal *terminal) {
    for (int cha = 0; cha < MaxChannelCount; cha++) {
        shaChannelClear(terminal->inputChannel+cha);
    }
    while (1) {
        ShaLink *link = terminal->firstLink;
        if (link == NULL) break;
        shaLinkExtruct(link);
        shaLinkFree(link);
    }
    shaPoolClear(&terminal->inputPool);
    shaPoolClear(&terminal->outputPool);
}
