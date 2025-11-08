#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

ShaPacket* getPacketsScane(ShaTerminal *terminal, uint8_t channel, int fun) {
    uint8_t bestChannel = 0;
    MCS bestAge = 0;
    ShaPacket *bestPacket = NULL;
    int fromCha = (fun > 0) ? channel : 0;
    int toCha = (fun > 0) ? channel : MaxChannelCount-1;
    for (int cha = fromCha; cha <= toCha; cha++) {
        while (1) {
            ShaPacket *packet = terminal->inputChannel[cha].firstPacket;
            if (packet == NULL) break;
            MCS now = GetNow();
            if (!packet->storedAt && now - packet->createdAt > terminal->ParmMaxLatency*2) {
                printf("%ld: Purge unfilled packet %d\n", GetNow(), packet->indexPacket);
                shaPacketExtruct(terminal, packet);
                shaPacketFree(packet);
                continue;
            }
            if (packet->storedAt) {
                if (bestPacket == NULL || packet->createdAt < bestAge) {
                    bestChannel = cha;
                    bestAge = packet->createdAt;
                    bestPacket = packet;
                }
            }
            break;
        }
    }
    if (bestPacket == NULL || fun < 0) return NULL;
    shaPacketExtruct(terminal, bestPacket);
    terminal->statistic.recvDataPackets++;
    return bestPacket;
}

void shaChannelStep(ShaTerminal *term) {
    getPacketsScane(term, 0, -1);
}

ShaPacket* shaInputGetPacket(ShaTerminal *terminal) {
    return getPacketsScane(terminal, 0, 0);
}

ShaPacket* shaInputGetChannel(ShaTerminal *terminal, uint8_t channel) {
    return getPacketsScane(terminal, channel, 1);
}

