package shalink

import (
	"math/rand"
	"time"
)

func (terminal *Terminal) outChunking(packet *Packet) []*Chunk {
	terminal.gateOut.Lock()
	defer terminal.gateOut.Unlock()

	var chunks []*Chunk
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
		chunk.head.indexPacket = packet.Index
		chunk.head.offsetPacket = uint32(offset)
		chunk.head.sizePacket = uint32(size)
		chunk.sizeData = chunkSize
		copy(chunk.data[:], packet.Data[offset:offset+chunkSize])
		chunk.head.indexChunk = terminal.outChunks.index
		terminal.outChunks.index++
		chunks = append(chunks, chunk)
		terminal.outInsertChunk(chunk)
		offset += chunkSize
		terminal.statistic.OutChunksData.Inc()
		terminal.statistic.OutBytesData.Add(float64(chunkSize))
		terminal.statistic.DebugChunks++
		terminal.statistic.DebugBytes += int64(chunkSize)
	}

	return chunks
}

func (terminal *Terminal) outSynchroSend(synch Synchro) {
	data := synchroToBytes(synch)
	chunk := allocChunk()
	chunk.head.offsetPacket = offsetSignSynch
	chunk.sizeData = len(data)
	copy(chunk.data[:], data)
	terminal.sendChunks([]*Chunk{chunk})
	chunk.Free()
}

func (terminal *Terminal) outInsertChunk(chunk *Chunk) {
	terminal.outChunks.insertLast(chunk)
}

func (terminal *Terminal) outFoundChunk(index uint32) *Chunk {
	terminal.gateOut.Lock()
	defer terminal.gateOut.Unlock()

	for chunk := terminal.outChunks.last; chunk != nil; chunk = chunk.pred {
		if chunk.head.indexChunk == index {
			return chunk
		}
	}
	return nil
}

func (terminal *Terminal) goOutSynch() {
	terminal.gateOut.Lock()
	defer terminal.gateOut.Unlock()

	terminal.outControlChunks()
	terminal.nextOutSynch = time.Now().Add(periodSynchro * 100 / time.Duration(90+rand.Intn(20)))
	terminal.isOutSynch = false
}

func (terminal *Terminal) outControlChunks() {
	now := time.Now()
	for {
		chunk := terminal.outChunks.first
		if chunk == nil || chunk.liveTo.After(now) {
			break
		}
		terminal.outChunks.extract(chunk)
		chunk.Free()
	}
}
