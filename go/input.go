package shalink

import (
	"time"
)

type itInput struct {
	channel   int
	index     int64
	size      int64
	createdAt time.Time

	nextInput  *itInput
	firtsInput *itInput
	firstChunk *ItChunk
	lastChunk  *ItChunk
}

func (terminal *ItTerminal) InputChunk(chunk *ItChunk) {
	packet := BuildPacket(chunk.channel, chunk.index, chunk.data)
	terminal.toRead <- packet
}
