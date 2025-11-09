#ifndef _INTEROP_H
#define _INTEROP_H

#include "terminal.h"

// Lock all terminal structures
void shaLock(ShaTerminal *terminal);
// Unlock all terminal structures
void shaUnlock(ShaTerminal *terminal);

// Receive input chunk for packeting
void shaInputData(ShaTerminal *terminal, ShaChunk *chunk);
// Control input pool
void shaInputStep(ShaTerminal *terminal);
// Find packet for the input chunk
ShaPacket* shaPacketFind(ShaTerminal *terminal, ShaChunk *chunk, int need);

// Control output pool
void shaOutputStep(ShaTerminal *terminal);
// Send to output data bytes
void shaOutputData(ShaTerminal *terminal, uint8_t channel, const void *data, uint32_t size);

// Build input packet for channel and size
ShaPacket* shaBuildPacket(uint8_t channel, uint32_t indexChannel, uint32_t sizeData);
// Control input packets
void shaChannelStep(ShaTerminal *terminal);
// Store the filled packet
void shaPacketStore(ShaTerminal *terminal, ShaPacket *packet);
// Insert the packet to channel collection
void shaPacketInsert(ShaTerminal *term, ShaPacket *packet);
// Extruct the packet from channel collection
void shaPacketExtruct(ShaTerminal *term, ShaPacket *packet);
// Free the packet
void shaPacketFree(ShaPacket *packet);

// Get first ready packet from any input collection
ShaPacket* shaInputGetPacket(ShaTerminal *terminal);
// Get first ready packet from channel collection
ShaPacket* shaInputGetChannel(ShaTerminal *terminal, uint8_t channel);

// Build link to address/port
ShaLink* shaBuildLink(ShaTerminal *terminal, const char *address, int port, int isServer);
// Insert the link to terminal
void shaLinkInsert(ShaTerminal *terminal, ShaLink *link);
// Extruct the link from terminal
void shaLinkExtruct(ShaLink *link);
// Step processing of link
void shaLinkStep(ShaLink *link);
// Open link connection
void shaLinkOpen(ShaLink *link);
// Add diapazon to output
void shaLinkOutputQueue(ShaLink *link, uint32_t firstChunk, uint32_t countChunk);
// Output chunk to link
void shaLinkOutputGuest(ShaLink *link, ShaGuest *guest, ShaChunk *chunk);
// Output code data
ssize_t shaLinkOutputCode(ShaLink *link, ShaGuest *guest, const void *code, int size);

// Processing the input connection
void shaLinkInput(ShaLink *link);
// Processing the output connection
void shaLinkOutput(ShaLink *link);
// Open the connection
void shaLinkOpen(ShaLink *link);
// Reset the connection
void shaLinkReset(ShaLink *link);

// Free link connection
void shaLinkFree(ShaLink *link);

// Build guest from address 
ShaLink* shaGuestBuild(struct sockaddr_in *addr);
// Insert the guest to link
void shaGuestInsert(ShaLink *link, ShaGuest *guest);
// Extruct the guest from link
void shaGuestExtruct(ShaLink *link, ShaGuest *guest);
// Find guest on listening server socket
ShaGuest* shaGuestFind(ShaLink *link, struct sockaddr_in *addr);
// Free guest to link
void shaGuestFree(ShaLink *link);

// Reply answer to request holes
void shaSyncInput(ShaLink *link, ShaGuest *guest, ShaChunk *chunk);
// Sunc output connection
void shaSyncOutput(ShaLink *link, ShaGuest *guest);

// Insert the chunk to pool
int shaPoolInsert(ShaPool *pool, ShaChunk *chunk);
// Append the chunk to pool
void shaPoolAppend(ShaPool *pool, ShaChunk *chunk);
// Extruct the chunk from pool
void shaPoolExtruct(ShaPool *pool, ShaChunk *chunk);
// Get first index if chunk
uint32_t shaPoolFirst(ShaPool *pool);
// Get count of chunks
uint32_t shaPoolCount(ShaPool *pool);
// Clear pool of chunks
void shaPoolClear(ShaPool *pool);

// Test for present chunks
int shaQueuePresent(ShaQueue *queue);
// Get/pop first chunks
uint32_t shaQueueGet(ShaQueue *queue);
// Add pot of chunks to queue
void shaQueueInsert(ShaQueue *queue, uint32_t firstChunk, uint32_t countChunk);

// Clear channel collect of packets
void shaChannelClear(ShaChannel *channel);

// Build chunk from parameters and data
ShaChunk* shaChunkBuildData(uint8_t channel, uint32_t indexPacket, uint32_t offsetPacket, uint32_t sizePacket, const void *data, uint16_t sizeData);
// Build chunk from input stream code
ShaChunk* shaChunkBuildCode(const void *code, uint32_t size);
// Build chunk for synchronize
ShaChunk* shaChunkBuildSync();
// Calcule right bound of chunk in packet
uint32_t shaChunkRight(ShaChunk *chunk);
// Compare overloading indexes
int shaCompare(uint32_t a, uint32_t b);
// Free chunk
void shaChunkFree(ShaChunk *chunk);

#endif
