#include "src/terminal.h"

const char *address = "127.0.0.1";
const int port = 9900;
const int senderAsServer = 0;
int isStoping = 0;

#define CountOfReceivers 1
#define CountOfPackets 100
#define PauseOfPackets ShaMSec*20
#define MaxSizePacket 100000
#define MaxDataSize MaxSizePacket-8
typedef struct InfoPacket {
    int64_t At;
    uint8_t Data[MaxDataSize];
} InfoPacket;

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
    TerminalAddLink(sender, address, port, senderAsServer);
    printf("Start %s\n", sender->name);
    Sleep(ShaSec);
    for (int number = 0; number < CountOfPackets; number++) {
        InfoPacket info;
        int size = MaxDataSize; //rand()%MaxDataSize;
        info.At = GetNow();
        for (int d = 0; d < size; d++) info.Data[d] = (d&0xff);
        TerminalSend(sender, 1, &info, size+8);
        //printf("Sent packet: %d\n", number+1);
        Sleep(PauseOfPackets);
    }
    Sleep(3*ShaSec);
    isStoping = 1;
    printStatistic(sender);
    TerminalFree(sender);
    return NULL;
}

// Receiver process
void* runReceiver(void *it) {
    ShaTerminal *receiver = (ShaTerminal*)it;
    TerminalAddLink(receiver, address, port, !senderAsServer);
    int number = 1;
    printf("Start %s\n", receiver->name);
    MCS delaySumm = 0;
    int delayCount = 0; 
    MCS lastPacketAt = GetNow();
    while (!isStoping || GetSience(lastPacketAt) < ShaSec) {
        ShaPacket *packet = TerminalGetPacket(receiver);
        if (packet != NULL) {
            lastPacketAt = GetNow();
            InfoPacket *info = (InfoPacket*)(packet->data);
            delaySumm = GetNow()-info->At;
            delayCount++;
            //printf("Received packet: %d\n", info->Number);
            if (packet->indexPacket != number) {
                if (packet->indexPacket == number+1) printf("ERROR: skiped packet %d\n", number);
                else printf("ERROR: skiped packets %d - %d\n", number, packet->indexPacket - 1);
            } else {
                int size = packet->sizeData-8;
                for (int d = 0; d < size; d++) {
                    if (info->Data[d] != (d&0xff)) {
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
    return NULL;
}

int main() {
    srand(time(NULL));
    // Start of sender
    ShaTerminal *sender = BuildTerminal("Sender");
    pthread_t senderId;
    pthread_create(&senderId, NULL, runSender, sender);
    for (int r = 0; r < CountOfReceivers; r++) {
        // Start of receiver
        char name[64];
        sprintf(name, "Receiver%d", r);
        ShaTerminal *receiver = BuildTerminal(name);
        pthread_t receiverId;
        pthread_create(&receiverId, NULL, runReceiver, receiver);
    }
    // Wait of stop signal of sender (100 sec)
    for (int step = 0; step < 100; step++) {
        if (isStoping) break;
        Sleep(ShaSec);
    }
    isStoping = 1;
    printf("Stoping...\n");
    // Wait of terminate of process
    Sleep(3*ShaSec);
    printf("Done\n");
}
