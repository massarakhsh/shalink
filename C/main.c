#include "src/terminal.h"

int modeTransmitter = 0;
int modeReceiver = 0;
int modeMirror = 0;
int modeExternal = 0;
int modeInvert = 0;
int modeHelp = 0;
const char *url = NULL;

int sizePackets = 1000;
int countPackets = 100;
MCS latency = 100 * ShaMSec;
MCS pausePackets = 10 * ShaMSec;

char urlAddress[256];
int urlPort = 0;

int screenHeight, screenWidth;

pthread_t transmitterId;
pthread_t receiverId;
pthread_t mirrorId;

// Print of statistic
void printStatistic(ShaTerminal *terminal) {
    ShaStatistic *stat = &terminal->statistic;
    printf("\n=== Statistic of %s\n", terminal->name);
    printf("Frames out/in/lost/break: %ld / %ld / %ld / %ld\n",
        stat->framesOut, stat->framesIn, stat->framesLost, stat->framesBreak);
    printf("Packets out/in/double/late: %ld / %ld / %ld / %ld\n",
        stat->packetsOut, stat->packetsIn, stat->packetsDouble, stat->packetsLate);
    printf("Packets attempts 1/2/3+: %ld / %ld / %ld\n",
        stat->packetsOut1, stat->packetsOut2, stat->packetsOut3);
    printf("Syncs out/in: %ld / %ld\n",
        stat->syncsOut, stat->syncsIn);
    printf("Bytes out/in: %ld / %ld\n",
        stat->bytesOut, stat->bytesIn);
    printf("Total packets out/in: %ld / %ld\n",
        stat->pksTotalOut, stat->pksTotalIn);
    printf("Total bytes out/in: %ld / %ld\n",
        stat->btsTotalOut, stat->btsTotalIn);
    printf("RTT (mcs) self/other: %ld / %ld\n", stat->meRTT, stat->youRTT);
}

// Transmitter process
void* runTransmitter(void *it) {
    ShaTerminal *terminal = (ShaTerminal*)it;
    TerminalSetLatency(terminal, latency);
    TerminalAddLink(terminal, urlAddress, urlPort, !modeInvert && !modeExternal);
    printf("Start %s\n", terminal->name);
    uint8_t *info = (uint8_t*)malloc(sizePackets);
    int number = 0;
    MCS lastPacketAt = GetNow();
    while (number <= countPackets) {
        if (number == 0 && TerminalIsReady(terminal)) number = 1;
        if (number > 0 && GetSience(lastPacketAt) >= pausePackets) {
            *(uint64_t*)info = GetNow();
            for (int d = 8; d < sizePackets; d++) info[d] = (d&0xff);
            TerminalSend(terminal, 0, 1, info, sizePackets);
            lastPacketAt = GetNow();
            number++;
        }
        Sleep(ShaMSec);
    }
    Sleep(ShaSec);
    transmitterId = 0;
    printStatistic(terminal);
    TerminalFree(terminal);
    return NULL;
}

// Receiver process
void* runReceiver(void *it) {
    ShaTerminal *terminal = (ShaTerminal*)it;
    TerminalSetLatency(terminal, latency);
    TerminalAddLink(terminal, urlAddress, urlPort, modeInvert && !modeExternal);
    int number = 0;
    printf("Start %s\n", terminal->name);
    MCS delaySumm = 0;
    int delayCount = 0; 
    MCS lastPacketAt = GetNow();
    while (number == 0 || GetSience(lastPacketAt) < 3*ShaSec) {
        if (number == 0 && TerminalIsReady(terminal)) {
            lastPacketAt = GetNow();
            number = 1;
        }
        ShaFrame *packet = (number > 0) ? TerminalGetPacket(terminal) : NULL;
        if (packet != NULL) {
            lastPacketAt = GetNow();
            MCS at = *(uint64_t*)packet->data;
            delaySumm += GetNow()-at;
            delayCount++;
            //printf("Received packet: %d\n", info->Number);
            if (packet->indexPacket != number) {
                if (packet->indexPacket == number+1) printf("ERROR: skiped packet %d\n", number);
                else printf("ERROR: skiped packets %d - %d\n", number, packet->indexPacket - 1);
            } else {
                int size = packet->sizeData;
                for (int d = 8; d < size; d++) {
                    if (packet->data[d] != (d&0xff)) {
                        printf("ERROR: data\n");
                        break;
                    }
                }
            }
            number = packet->indexPacket+1;
            PacketFree(packet);
        }
        Sleep(ShaMSec);
    }
    printStatistic(terminal);
    if (delayCount > 0) {
        printf("Delay average: %ld mcs\n", delaySumm / delayCount);
    }
    receiverId = 0;
    TerminalFree(terminal);
    return NULL;
}

// Mirror process
void* runMirror(void *it) {
    ShaTerminal *terminal = (ShaTerminal*)it;
    terminal->isMirror = 1;
    TerminalAddLink(terminal, urlAddress, urlPort, 1);
    printf("Start %s\n", terminal->name);
    while (!modeTransmitter || transmitterId > 0) {
        Sleep(ShaMSec);
    }
    mirrorId = 0;
    TerminalFree(terminal);
    return NULL;
}

int getInteger(const char *arg) {
    int number = -1;
    while (1) {
        uint8_t ch = *arg;
        if (ch==0) break;
        else if (ch >= '0' && ch <= '9') {
            if (number < 0) number = 0;
            number = number*10 + (ch-'0');
        }
        else return -1;
        arg++;
    }
    return number;
}

int main(int argc, const char **argv) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("=== ShaLink tester started\n");
    for (int a = 1; a < argc; a++) {
        const char *arg = argv[a];
        if (strcmp(arg, "-h") == 0) modeHelp = 1;
        else if (strcmp(arg, "-t") == 0) modeTransmitter = 1;
        else if (strcmp(arg, "-r") == 0) modeReceiver = 1;
        else if (strcmp(arg, "-e") == 0) modeExternal = 1;
        else if (strcmp(arg, "-m") == 0) modeMirror = 1;
        else if (strcmp(arg, "-i") == 0) modeInvert = 1;
        else if (strcmp(arg, "-l") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : -1;
            if (ival < 0) {
                printf("ERROR. Bad latency of stream: %s\n", arg);
                modeHelp = 1; break;
            }
            latency = ival * ShaMSec;
            a++;
        } else if (strcmp(arg, "-s") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : -1;
            if (ival <= 0) {
                printf("ERROR. Bad size of packets: %s\n", arg);
                modeHelp = 1; break;
            }
            sizePackets = ival;
            a++;
        } else if (strcmp(arg, "-c") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : -1;
            if (ival < 0) {
                printf("ERROR. Bad count of packets: %s\n", arg);
                modeHelp = 1; break;
            }
            countPackets = ival;
            a++;
        } else if (strcmp(arg, "-p") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : -1;
            if (ival < 0) {
                printf("ERROR. Bad pause between packets: %s\n", arg);
                modeHelp = 1; break;
            }
            pausePackets = ival * ShaMSec;
            a++;
        }
        else if (url == NULL) url = arg;
        else modeHelp = 1;
    }

    if (url != NULL) {
        const char *div = strchr(url, ':');
        if (div != NULL) {
            urlPort = getInteger(div+1);
            memset(urlAddress, 0, sizeof(urlAddress));
            memcpy(urlAddress, url, div-url);
        }
    }

    if (urlPort == 0) {
        printf("ERROR. Port must be present\n");
        modeHelp = 1;
    }
    if (!modeTransmitter && !modeReceiver && !modeMirror) {
        printf("ERROR. Transmitter, receiver or mirror mode must be present\n");
        modeHelp = 1;
    }

    if (modeHelp) {
        printf("Use: shalink [options] [address]:port\n");
        printf("-h - print this help\n");
        printf("-t - start transmitter (default as server)\n");
        printf("-r - start receiver (default as client)\n");
        printf("-m - start mirror server (with external clients)\n");
        printf("-e - use external mirror server\n");
        printf("-i - use invert mode (sender as client, receiver as server), not for mirror\n");
        printf("-l msec - latency of stream, default is 100\n");
        printf("-z size - size of packets, default is 10000\n");
        printf("-c number - count of packets, default is 100\n");
        printf("-p msec - pause between packets, default is 10 msec\n");

        return 1;
    }

    if (modeExternal) printf("Use external server.\n");
    if (modeInvert) printf("Invert mode, transmitter as client and receiver as server\n");
    if (modeTransmitter) {
        printf("Latency: %ld msec\n", latency/ShaMSec);
        printf("Send: %d packets\n", countPackets);
        printf("Size of packets: %d bytes\n", sizePackets);
        printf("Pause between: %ld msec\n", pausePackets/ShaMSec);
    }
    printf("\n");

    if (modeTransmitter) {
        // Start of transmitter
        ShaTerminal *transmitter = BuildTerminal("Transmitter");
        pthread_create(&transmitterId, NULL, runTransmitter, transmitter);
    }

    if (modeReceiver) {
        // Start of receiver
        ShaTerminal *receiver = BuildTerminal("Receiver");
        pthread_create(&receiverId, NULL, runReceiver, receiver);
    }

    if (modeMirror) {
        // Start of mirror
        ShaTerminal *mirror = BuildTerminal("Mirror");
        pthread_create(&mirrorId, NULL, runMirror, mirror);
    }

    // Wait of stop signal
    while (transmitterId > 0 || receiverId > 0 || mirrorId > 0) {
        Sleep(ShaSec);
    }

    printf("Done\n");
    return 0;
}
