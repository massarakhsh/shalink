#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

void linkInputChunk(ShaLink *link, ShaGuest *guest, ShaChunk *chunk) {
    ShaTerminal *terminal = link->terminal;
    terminal->statistic.recvTotalChunks++;
    terminal->statistic.recvTotalBytes += ChunkHeadSize + chunk->head.sizeData;
    if (chunk->head.proto == ChunkProtoData) {
        shaInputData(link->terminal, chunk);
    } else if (chunk->head.proto == ChunkProtoSync) {
        shaSyncInput(link, guest, chunk);
    } else if (chunk->head.proto == ChunkProtoRequest) {
    } else {
        shaChunkFree(chunk);
    }
}

void linkInputCode(ShaLink *link, ShaGuest *guest, const uint8_t *code, uint16_t size) {
    if (size < ChunkHeadSize) {
        printf("ERROR inputCode left=%d\n", (int)size);
        return;
    }
    const ChunkHead *head = (const ChunkHead*)code;
    uint16_t sizeData = head->sizeData;
    uint16_t sizeCode = ChunkHeadSize+sizeData;
    if (sizeCode > size) {
        printf("ERROR inputCode size=%d sizeCode=%d\n", (int)size, (int)sizeCode);
        return;
    }
    ShaChunk *chunk = shaChunkBuildCode(head, sizeCode);
    if (chunk == NULL) {
        printf("ERROR inputCode NO free chunk\n");
        return;
    }
    linkInputChunk(link, guest, chunk);
    return;
}

void linkInputMirror(ShaLink *link, ShaGuest *guest, const uint8_t *code, uint16_t size) {
    for (ShaGuest *toGuest = link->firstGuest; toGuest != NULL; toGuest = toGuest->nextGuest) {
        if (toGuest != guest) {
            shaLinkOutputCode(link, toGuest, code, size);
        }
    }
    return;
}

void shaLinkInput(ShaLink *link) {
    struct pollfd pfd;
    pfd.fd = link->socketId;
    pfd.events = POLLIN | POLLERR;
    pfd.revents = 0;  
    int result = poll(&pfd, 1, 1);
    
    if (result < 0) {
        printf("Input pool error\n");
        shaLinkReset(link);
        return;
    }

    if (pfd.revents & POLLIN) {
        ssize_t in_size = 0;
        ShaGuest *guest = NULL;
        if (link->isServer) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            in_size = recvfrom(link->socketId, link->in_buffer, LinkBufferSize, 0, (struct sockaddr*)&client_addr, &client_len);
            if (in_size > 0) {
                guest = shaGuestFind(link, &client_addr);
            }
        } else {
            in_size = recv(link->socketId, link->in_buffer, LinkBufferSize, 0);
        }
        if (in_size <= 0) {
            // Закрываем соединение
            printf("Link dislinked: %s\n", link->address);
            shaLinkReset(link);
        } else if (link->terminal->isMirror) {
            linkInputMirror(link, guest, link->in_buffer, in_size);
        } else {
            linkInputCode(link, guest, link->in_buffer, in_size);
        }
    }
}
