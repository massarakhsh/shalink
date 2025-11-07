#include "src/terminal.h"

int modeServer = 0;
int modeClient = 0;
int modeReverse = 0;
int modeHelp = 0;
const char *url = NULL;

int sizePackets = 8192;
int countPackets = 100;
MCS pausePackets = 20 * ShaMSec;

char address[256];
int port = 0;
int useSender = 0;
int useReceiver = 0;
int isStarting = 0;
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
    TerminalAddLink(sender, address, port, !modeReverse);
    printf("Start %s\n", sender->name);
    Sleep(ShaSec);
    uint8_t *info = (uint8_t*)malloc(sizePackets);
    for (int number = 0; number < countPackets; number++) {
        *(uint64_t*)info = GetNow();
        for (int d = 8; d < sizePackets; d++) info[d] = (d&0xff);
        TerminalSend(sender, 1, info, sizePackets);
        Sleep(pausePackets);
    }
    Sleep(2*ShaSec);
    isStoping = 1;
    printStatistic(sender);
    TerminalFree(sender);
    return NULL;
}

// Receiver process
void* runReceiver(void *it) {
    ShaTerminal *receiver = (ShaTerminal*)it;
    TerminalAddLink(receiver, address, port, modeReverse);
    int number = 1;
    printf("Start %s\n", receiver->name);
    MCS delaySumm = 0;
    int delayCount = 0; 
    MCS lastPacketAt = GetNow();
    while (!isStoping && !isStarting || GetSience(lastPacketAt) < ShaSec) {
        ShaPacket *packet = TerminalGetPacket(receiver);
        if (packet != NULL) {
            isStarting = 1;
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
    if (!useSender) isStoping = 1;
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
        else if (strcmp(arg, "-s") == 0) modeServer = 1;
        else if (strcmp(arg, "-c") == 0) modeClient = 1;
        else if (strcmp(arg, "-r") == 0) modeReverse = 1;
        else if (strcmp(arg, "-p") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : 0;
            if (ival == 0) { modeHelp = 1; break;}
            sizePackets = ival;
            a++;
        } else if (strcmp(arg, "-n") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : 0;
            if (ival == 0) { modeHelp = 1; break;}
            countPackets = ival;
            a++;
        } else if (strcmp(arg, "-t") == 0) {
            int ival = (a+1 < argc) ? getInteger(argv[a+1]) : 0;
            if (ival == 0) { modeHelp = 1; break;}
            pausePackets = ival * ShaMSec;
            a++;
        }
        else if (url == NULL) url = arg;
        else modeHelp = 1;
    }

    if (url != NULL) {
        const char *div = strchr(url, ':');
        if (div != NULL) {
            port = getInteger(div+1);
            memset(address, 0, sizeof(address));
            memcpy(address, url, div-url);
        }
    }

    if (port == 0) modeHelp = 1;
    if (!modeServer && !modeClient) modeHelp = 1;

    if (modeHelp) {
        printf("Use: shalink [options] address\n");
        printf("-h - print this help\n");
        printf("-s - start server, sender\n");
        printf("-c - start client, reciever\n");
        printf("-r - reverse mode (client as sender, server as reciever)\n");
        printf("-p size - size of packets, default is 8192\n");
        printf("-t msec - temp of packets, default is 20 msec\n");
        printf("-n number - count of packets, default is 100\n");

        return 1;
    }

    useSender = (modeServer && !modeReverse) || (modeClient && modeReverse);
    useReceiver = (modeServer && modeReverse) || (modeClient && !modeReverse);

    if (useSender) {
        // Start of sender
        ShaTerminal *sender = BuildTerminal("Sender");
        pthread_t senderId;
        pthread_create(&senderId, NULL, runSender, sender);
    }

    if (useReceiver) {
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
