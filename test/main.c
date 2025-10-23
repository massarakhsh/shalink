#include "../src/terminal.h"

const char *address = "127.0.0.1";
//const char *address = "192.168.17.108";
const int port = 9900;
const int senderIsServer = 1;

pthread_t senderId;
pthread_t receiverId;
int isStoping = 0;

#define MaxDataSize 65536

typedef struct Informa {
    int32_t Number;
    int32_t Size;
    uint8_t Data[MaxDataSize];
} Informa;

void* runSender(void *it) {
    srand(time(NULL));
    Terminal *sender = BuildTerminal("Sender");
    TerminalAddLink(sender, address, port, senderIsServer);
    printf("Start %s\n", sender->name);
    Sleep(1000);
    for (int number = 1; number <= 100; number++) {
        Informa info;
        info.Number = number;
        int size = rand()%MaxDataSize;
        info.Size = size;
        for (int d = 0; d < size; d++) info.Data[d] = (d&0xff);
        TerminalSend(sender, 1, &info, 8+size);
    }
    Sleep(1000);
    isStoping = 1;
    TerminalFree(sender);
    return NULL;
}

void* runReceiver(void *it) {
    int r = *(int*)it;
    char name[16];
    sprintf(name, "Receiver%d", r);
    Terminal *receiver = BuildTerminal(name);
    TerminalAddLink(receiver, address, port, !senderIsServer);
    int number = 1;
    printf("Start %s\n", receiver->name);
    while (!isStoping) {
        Packet *packet = TerminalGet(receiver);
        if (packet != NULL) {
            Informa *info = (Informa*)(packet->data);
            printf("Received: %d\n", info->Number);
            if (info->Number != number) {
                printf("ERROR: number=%d, need=%d\n", info->Number, number);
            } if (info->Size+8 != packet->sizeData) {
                printf("ERROR: size=%d, need=%d\n", info->Size+8, packet->sizeData);
            } else {
                for (int d = 0; d < info->Size; d++) {
                    if (info->Data[d] != (d&0xff)) {
                        printf("ERROR: data\n");
                        break;
                    }
                }
            }
            number = info->Number+1;
            PacketFree(packet);
        }
        Sleep(1);
    }
    TerminalFree(receiver);
    return NULL;
}

int main() {
    pthread_create(&senderId, NULL, runSender, NULL);
    int recs[3] = {1, 2, 3};
    for (int r = 0; r < 1; r++) {
        pthread_create(&receiverId, NULL, runReceiver, recs+r);
    }
    for (int step = 0; step < 10; step++) {
        if (isStoping) break;
        usleep(1000000);
    }
    isStoping = 1;
    printf("Stoping...\n");
    usleep(1000000);
    printf("Done\n");
}
