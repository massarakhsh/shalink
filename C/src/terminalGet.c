#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

ShaFrame* getPacketsScane(ShaTerminal *terminal, uint8_t channel, int fun) {
    MCS bestAge = 0;
    ShaFrame *bestPacket = NULL;
    int fromCha = (fun > 0) ? channel : 0;
    int toCha = (fun > 0) ? channel : MaxChannelCount-1;
    for (int cha = fromCha; cha <= toCha; cha++) {
        while (1) {
            ShaFrame *packet = terminal->inputChannel[cha].firstFrame;
            if (packet == NULL) break;
            if (packet->storedAt) {
                if (bestPacket == NULL || packet->createdAt < bestAge) {
                    bestAge = packet->createdAt;
                    bestPacket = packet;
                }
            }
            break;
        }
    }
    if (bestPacket == NULL || fun < 0) return NULL;
    shaPacketExtruct(terminal, bestPacket);
    terminal->statistic.framesIn++;
    return bestPacket;
}

ShaFrame* shaInputGetPacket(ShaTerminal *terminal) {
    return getPacketsScane(terminal, 0, 0);
}

ShaFrame* shaInputGetChannel(ShaTerminal *terminal, uint8_t channel) {
    return getPacketsScane(terminal, channel, 1);
}

void shaChannelControl(ShaTerminal *terminal) {
    for (int cha = 0; cha < MaxChannelCount; cha++) {
        while (1) {
            ShaFrame *packet = terminal->inputChannel[cha].firstFrame;
            if (packet == NULL) break;
            MCS now = GetNow();
            if (!packet->storedAt && now > packet->liveAt) {
                printf("%ld: Purge unfilled packet %d\n", GetNow(), packet->indexPacket);
                shaPacketExtruct(terminal, packet);
                shaPacketFree(packet);
                continue;
            }
            break;
        }
    }
}

