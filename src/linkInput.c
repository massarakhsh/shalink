#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

void linkInputCode(Link *link, const uint8_t *code, uint16_t size) {
    uint32_t offset = 0;
    uint16_t left = size;
    while (left) {
        if (left < ChunkHeadSize) {
            printf("ERROR inputCode left=%d\n", (int)left);
            break;
        }
        const ChunkHead *head = (const ChunkHead*)(code+offset);
        uint16_t sizeData = head->sizeData;
        uint16_t sizeCode = ChunkHeadSize+sizeData;
        if (sizeCode > left || sizeCode > DtgMaxInput) {
            printf("ERROR inputCode offset=%d, left=%d sizeData=%d maxInput=%d\n", offset, (int)left, (int)sizeData, DtgMaxInput);
            break;
        }
        Chunk *chunk = buildChunkCode(head, sizeCode);
        if (chunk == NULL) {
            printf("ERROR inputCode NO free chunk\n");
            break;
        }
        terminalInputChunk(link->terminal, chunk);
        link->terminal->stepPauseMcs = 0;
        offset += sizeCode;
        left -= sizeCode;
    }
}

void linkInput(Link *link) {
    struct pollfd pfd;
    pfd.fd = link->socketId;
    pfd.events = POLLIN | POLLERR;
    pfd.revents = 0;  
    int result = poll(&pfd, 1, 1);
    
    if (result < 0) {
        printf("Input pool error\n");
        linkReset(link);
        return;
    }

    if (pfd.revents & POLLIN) {
        uint8_t in_buffer[DtgMaxInput];
        ssize_t in_size = 0;
        if (link->isServer) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            in_size = recvfrom(link->socketId, in_buffer, sizeof(in_buffer), 0, (struct sockaddr*)&client_addr, &client_len);
            if (in_size > 0) {
                Guest *guest = linkFindGuest(link, &client_addr);
                if (guest != NULL) {
                }
            }
        } else {
            in_size = recv(link->socketId, in_buffer, sizeof(in_buffer), 0);
        }
        if (in_size > 0) {
            linkInputCode(link, in_buffer, in_size);
        } else {
            // Закрываем соединение
            printf("Client dislinked\n");
            linkReset(link);
        }
    }
}
