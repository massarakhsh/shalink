#ifndef _INTEROP_H
#define _INTEROP_H

#include "terminal.h"

void terminalLock(Terminal *terminal);
void terminalUnlock(Terminal *terminal);
void runTerminal(Terminal *terminal);
void terminalChunkClear(Terminal *terminal);
void terminalChunkInputData(Terminal *terminal, Chunk *chunk);
Packet* terminalFindPacket(Terminal *terminal, const ChunkHead *head);
void terminalChunkAppend(Terminal *terminal, Chunk *chunk);
void terminalInputChunk(Terminal *terminal, Chunk *chunk);

void terminalSend(Terminal *terminal, uint8_t channel, const void *data, uint16_t size);
void terminalPacketClear(Terminal *terminal);
void terminalPacketStore(Terminal *terminal, Packet *packet);
void terminalPacketInsert(Terminal *term, Packet *packet);
void terminalPacketExtruct(Terminal *term, Packet *packet);

Link* buildLink(Terminal *terminal, const char *address, int port, int isServer);
void linkStep(Link *link);
Guest* linkFindGuest(Link *link, struct sockaddr_in *addr);
void linkOpenBind(Link *link);
void linkOpenConnect(Link *link);

void terminalLinkInsert(Terminal *terminal, Link *link);
void terminalLinkExtruct(Terminal *terminal, Link *link);
void terminalLinkRoll(Terminal *terminal);

void terminalHolePacket(Terminal *term, Packet *packet);
void terminalHoleControl(Terminal *term, Packet *packet, Chunk *chunk);
void terminalHoleOutput(Terminal *term, Chunk *chunk);

Chunk* buildChunkData(uint8_t channel, uint32_t indexChannel, uint32_t offsetPacket, uint32_t sizePacket, const void *data, uint16_t sizeData);
Chunk* buildChunkCode(const void *code, uint32_t size);
uint32_t chunkRight(Chunk *chunk);
void chunkFree(Chunk *chunk);

int ioCompareIndex(uint32_t a, uint32_t b);
int ioSetNonblocking(int sockfd);
int ioProbeConnection(int sockfd);

void linkInput(Link *link);
void linkOutput(Link *link);
void openLink(Link *link);
void linkReset(Link *link);

Packet* buildPacket(uint8_t channel, uint32_t indexChannel, uint32_t sizeData);
void packetClearChunks(Packet *packet);
void packetFree(Packet *packet);

#endif
