package shalink

import "time"

func (terminal *Terminal) outChunking(packet *Packet) []Chunk {
	terminal.outChunkGate.Lock()
	defer terminal.outChunkGate.Unlock()

	var chunks []Chunk
	latency := terminal.getLatency(packet)
	offset := 0
	size := len(packet.Data)
	for offset < size {
		chunkSize := size - offset
		if chunkSize > chunkInfoSize {
			chunkSize = chunkInfoSize
		}
		chunk := allocChunk()
		chunk.liveTo = time.Now().Add(latency)
		chunk.head.proto = chunkProtoData
		chunk.head.channel = uint8(packet.Channel)
		chunk.head.indexPacket = packet.Index
		chunk.head.offsetPacket = uint32(offset)
		chunk.head.sizePacket = uint32(size)
		chunk.head.sizeData = uint16(chunkSize)
		copy(chunk.data[:], packet.Data[offset:offset+chunkSize])
		chunk.head.indexChunk = terminal.outChunkIndex
		terminal.outChunkIndex++
		chunks = append(chunks, *chunk)
		terminal.outInsertChunk(chunk)
		offset += chunkSize
	}
	terminal.outClearChunks()

	return chunks
}

func (terminal *Terminal) outInsertChunk(chunk *Chunk) {
	terminal.outChunks.insertLast(chunk)
	terminal.statistic.OutChunkQueue.SetValueInt("", int64(terminal.outChunks.count))
}

func (terminal *Terminal) outClearChunks() {
	now := time.Now()
	for {
		chunk := terminal.outChunks.first
		if chunk == nil || chunk.liveTo.After(now) {
			break
		}
		terminal.outChunks.extract(chunk)
		if terminal.outChunks.first != nil {
			terminal.outChunkTo = terminal.outChunks.first.liveTo
		}
		terminal.statistic.OutChunkQueue.SetValueInt("", int64(terminal.outChunks.count))
		freeChunk(chunk)
	}
}
