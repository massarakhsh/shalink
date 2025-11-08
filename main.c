#include "src/terminal.h"

int modeSender = 0;
int modeReceiver = 0;
int modeInvert = 0;
int modeHelp = 0;
const char *url = NULL;

int sizePackets = 10000;
int countPackets = 100;
MCS latency = 100 * ShaMSec;
MCS pausePackets = 10 * ShaMSec;

char urlAddress[256];
int urlPort = 0;
int isStoping = 0;

// Print of statistic
void printStatistic(ShaTerminal *terminal) {
    ShaStatistic *stat = &terminal->statistic;
    printf("=== Statistic of %s\n", terminal->name);
    printf("Sent packets data/total: %ld / %ld\n", stat->sendDataPackets, stat->sendTotalPackets);
    printf("Sent bytes data/total: %ld / %ld\n", stat->sendDataBytes, stat->sendTotalBytes);
    printf("Sent chunks data/repeat/sync/total: %ld / %ld / %ld / %ld\n", stat->sendDataChunks, stat->sendRepeatChunks, stat->sendSyncChunks, stat->sendTotalChunks);
    printf("Received packets data/total: %ld / %ld\n", stat->recvDataPackets, stat->recvTotalPackets);
    printf("Received bytes data/total: %ld / %ld\n", stat->recvDataBytes, stat->recvTotalBytes);
    printf("Received chunks data/repeat/sync/total: %ld / %ld / %ld / %ld\n", stat->recvDataChunks, stat->recvRepeatChunks, stat->recvSyncChunks, stat->recvTotalChunks);
    printf("RTT (mcs) me/you: %ld / %ld\n", stat->meRTT, stat->youRTT);
}

// Sender process
void* runSender(void *it) {
    ShaTerminal *sender = (ShaTerminal*)it;
    sender->ParmMaxLatency = latency;
    TerminalAddLink(sender, urlAddress, urlPort, !modeInvert);
    printf("Start %s\n", sender->name);
    uint8_t *info = (uint8_t*)malloc(sizePackets);
    int number = 0;
    MCS lastPacketAt = GetNow();
    while (!isStoping && number <= countPackets) {
        if (number == 0 && TerminalIsReady(sender)) number = 1;
        if (number > 0 && GetSience(lastPacketAt) >= pausePackets) {
            *(uint64_t*)info = GetNow();
            for (int d = 8; d < sizePackets; d++) info[d] = (d&0xff);
            TerminalSend(sender, 1, info, sizePackets);
            lastPacketAt = GetNow();
            number++;
        }
        Sleep(ShaMSec);
    }
    Sleep(ShaSec);
    isStoping = 1;
    printStatistic(sender);
    TerminalFree(sender);
    return NULL;
}

// Receiver process
void* runReceiver(void *it) {
    ShaTerminal *receiver = (ShaTerminal*)it;
    receiver->ParmMaxLatency = latency;
    TerminalAddLink(receiver, urlAddress, urlPort, modeInvert);
    int number = 0;
    printf("Start %s\n", receiver->name);
    MCS delaySumm = 0;
    int delayCount = 0; 
    MCS lastPacketAt = GetNow();
    while (!isStoping && number == 0 || GetSience(lastPacketAt) < ShaSec) {
        if (number == 0 && TerminalIsReady(receiver)) {
            lastPacketAt = GetNow();
            number = 1;
        }
        ShaPacket *packet = (number > 0) ? TerminalGetPacket(receiver) : NULL;
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
    printStatistic(receiver);
    if (delayCount > 0) {
        printf("Delay average: %ld mcs\n", delaySumm / delayCount);
    }
    TerminalFree(receiver);
    if (!modeSender) isStoping = 1;
    return NULL;
}

int getInteger(const char *arg) {
    int number = 0;
    while (1) {
        uint8_t ch = *arg;
        if (ch==0) break;
        else if (ch >= '0' && ch <= '9') number = number*10 + (ch-'0');
        else return 0;
        arg++;
    }
    return number;
}

int main(int argc, const char **argv) {
    printf("=== ShaLink tester started\n");
    for (int a = 1; a < argc; a++) {
        const char *arg = argv[a];
        if (strcmp(arg, "-h") == 0) modeHelp = 1;
        else if (strcmp(arg, "-s") == 0) modeSender = 1;
        else if (strcmp(arg, "-r") == 0) modeReceiver = 1;
        else if (strcmp(arg, "-i") == 0) modeInvert = 1;
        else if (strcmp(arg, "-l") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : 0;
            if (ival == 0) {
                printf("ERROR. Bad latency of stream: %s\n", arg);
                modeHelp = 1; break;
            }
            latency = ival * ShaMSec;
            a++;
        } else if (strcmp(arg, "-z") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : 0;
            if (ival == 0) {
                printf("ERROR. Bad size of packets: %s\n", arg);
                modeHelp = 1; break;
            }
            sizePackets = ival;
            a++;
        } else if (strcmp(arg, "-n") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : 0;
            if (ival == 0) {
                printf("ERROR. Bad count of packets: %s\n", arg);
                modeHelp = 1; break;
            }
            countPackets = ival;
            a++;
        } else if (strcmp(arg, "-p") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : 0;
            if (ival == 0) {
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
    if (!modeSender && !modeReceiver) {
        printf("ERROR. Sender or receiver bode must be present\n");
        modeHelp = 1;
    }

    if (modeHelp) {
        printf("Use: shalink [options] [address]:port\n");
        printf("-h - print this help\n");
        printf("-s - start sender (default as server)\n");
        printf("-r - start receiver (default as client)\n");
        printf("-i - invert mode (sender as client, receiver as server)\n");
        printf("-l msec - latency of stream, default is 100\n");
        printf("-z size - size of packets, default is 10000\n");
        printf("-n number - count of packets, default is 100\n");
        printf("-p msec - pause between packets, default is 10 msec\n");

        return 1;
    }

    if (modeSender+modeReceiver) printf("Loop mode (sender+receiver).\n");
    else if (modeSender) printf("Sender mode.\n");
    else if (modeReceiver) printf("Receiver mode.\n");
    if (modeInvert) printf("Invert mode, sender as client and receiver as server\n");
    if (modeSender) {
        printf("Latency: %ld msec\n", latency/ShaMSec);
        printf("Send: %d packets\n", countPackets);
        printf("Size of packets: %d bytes\n", sizePackets);
        printf("Pause between: %ld msec\n", pausePackets/ShaMSec);
    }

    if (modeSender) {
        // Start of sender
        ShaTerminal *sender = BuildTerminal("Sender");
        pthread_t senderId;
        pthread_create(&senderId, NULL, runSender, sender);
    }

    if (modeReceiver) {
        // Start of receiver
        ShaTerminal *receiver = BuildTerminal("Receiver");
        pthread_t receiverId;
        pthread_create(&receiverId, NULL, runReceiver, receiver);
    }

    // Wait of stop signal
    while (!isStoping) {
        Sleep(ShaSec);
    }
    isStoping = 1;
    printf("Stoping...\n");
    // Wait of terminate of process
    Sleep(3*ShaSec);
    printf("Done\n");
}
