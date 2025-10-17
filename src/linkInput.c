#include "terminal.h"
#include "interop.h"
#include <arpa/inet.h>
#include <poll.h>

void linkInputCode(Link *link, const uint8_t *code, uint16_t size) {
    while (size>0) {
        if (size < ChunkHeadSize) {
            printf("ERROR inputCode size=%d\n", (int)size);
            break;
        }
        uint16_t sizeData = *(const uint16_t*)(code+2);
        uint16_t sizeCode = ChunkHeadSize+sizeData;
        if (sizeCode > size || sizeCode > DtgMaxInput) {
            printf("ERROR inputCode size=%d sizeData=%d maxInput=%d\n", (int)size, (int)sizeData, DtgMaxInput);
            break;
        }
        Chunk *chunk = buildChunkCode(code, sizeCode);
        if (chunk == NULL) {
            printf("ERROR inputCode NO free chunk\n");
            break;
        }
        terminalChunkInput(link->terminal, chunk);
        code += sizeCode;
        size -= sizeCode;
    }
}

void linkInput(Link *link) {
    struct pollfd pfds[1+MaxGuestCount];
    
    pfds[0].fd = link->socketId;
    pfds[0].events = POLLIN;
    
    for (int g = 0; g < link->guestCount; g++) {
        pfds[1+g].fd = link->guests[g].socketId;
        pfds[1+g].events = POLLIN;
    }
    
    int result = poll(pfds, 1+link->guestCount, 1);
    
    if (result < 0) {
        printf("Input pool error\n");
        linkReset(link);
        return;
    }
    
    for (int g = 0; g < 1+link->guestCount; g++) {
        struct pollfd *pfd = pfds + g;
        if (pfd->revents & POLLIN) {
            //printf("Input pool %s\n", conn->address);
            if (link->isServer && g == 0) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                char ip_str[INET_ADDRSTRLEN];                

                int client_sock = accept(link->socketId, (struct sockaddr*)&client_addr, &client_len);
                if (client_sock < 0) continue;
                if (inet_ntop(AF_INET, &client_addr, ip_str, INET_ADDRSTRLEN) == NULL) {
                    close(client_sock);
                    continue;
                }
                //printf("Input IP: %s, Port: %d\n", ip_str, client_addr.sin_port);
                Guest *guest = findGuest(link, &client_addr);
                if (guest != NULL) {
                    close(guest->socketId);
                    guest->socketId = client_sock;
                    continue;
                }
                if (link->guestCount >= MaxGuestCount) {
                    close(client_sock);
                    continue;
                }

                ioSetNonblocking(client_sock);
                guest = link->guests+link->guestCount;                
                link->guestCount++;
                memset(guest, 0, sizeof(Guest));
                guest->addr = client_addr;
                guest->socketId = client_sock;
                guest->lastIn = GetNow();
                printf("New client linked: fd=%d\n", client_sock);
            } else {
                int client_sock = pfd->fd;
                char buffer[DtgMaxInput];
                ssize_t bytes_read = recv(client_sock, buffer, sizeof(buffer), 0);

                if (bytes_read <= 0) {
                    // Закрываем соединение
                    close(client_sock);
                    printf("Client dislinked\n");
                } else {
                    // Обрабатываем данные
                    linkInputCode(link, (const uint8_t*)buffer, bytes_read);
                }
            }
        }
    }
}

