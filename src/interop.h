#ifndef _INTEROP_H
#define _INTEROP_H

#include "terminal.h"

void terminalLock(Terminal *terminal);
void terminalUnlock(Terminal *terminal);
void runTerminal(Terminal *terminal);
void terminalChunkClear(Terminal *terminal);
void terminalChunkInputData(Terminal *terminal, Chunk *chunk);
Packet* terminalFindPacket(Terminal *terminal, Chunk *chunk);
Packet* terminalInputPacket(Terminal *terminal);

void terminalSend(Terminal *terminal, uint8_t channel, const void *data, uint16_t size);
void terminalPacketClear(Terminal *terminal);
void terminalPacketStore(Terminal *terminal, Packet *packet);
void terminalPacketInsert(Terminal *term, Packet *packet);
void terminalPacketExtruct(Terminal *term, Packet *packet);

Link* buildLink(Terminal *terminal, const char *address, int port, int isServer);
void terminalLinkInsert(Terminal *terminal, Link *link);
void terminalLinkExtruct(Terminal *terminal, Link *link);
void terminalLinkRoll(Terminal *terminal);
void linkStep(Link *link);

Chunk* buildChunkData(uint8_t channel, uint32_t indexChunk, uint32_t indexChannel, uint32_t offsetPacket, uint32_t sizePacket, const void *data, uint16_t sizeData);
Chunk* buildChunkCode(const void *code, uint32_t size);
void chunkFree(Chunk *chunk);
int chunkSend(Chunk *chunk, int sockfd);

int ioSetNonblocking(int sockfd);
int ioProbeConnection(int sockfd);

void linkInput(Link *link);
void outputLink(Link *link);
void openLink(Link *link);
void linkReset(Link *link);

Packet* buildPacket(uint8_t channel, uint32_t indexChannel, uint32_t sizeData);
void packetClearChunks(Packet *packet);
void packetFree(Packet *packet);

#define LOW 0x40000000;
#define MED 0x80000000;
#define HI 0xc0000000;
#define BEFORE(A,B) ((A)<(B))

#endif
