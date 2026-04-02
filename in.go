package shalink

import (
	"time"

	"github.com/massarakhsh/lik/log"
)

func (terminal *Terminal) inChunk(chunk *Chunk) {
	terminal.inChunkGate.Lock()
	defer terminal.inChunkGate.Unlock()

	if !terminal.inProbeActual(chunk) {
		freeChunk(chunk)
		return
	}
	chunk.liveTo = time.Now().Add(time.Second * 5)
	if !terminal.inInsertChunk(chunk) {
		freeChunk(chunk)
		return
	}
	packet := terminal.inInsertPacket(chunk)
	if packet == nil {
		return
	}
	if packet.collectData >= packet.sizeData {
		terminal.inReadyPacket(packet)
	}
}

func (terminal *Terminal) inProbeActual(chunk *Chunk) bool {
	return true
}

func (terminal *Terminal) inInsertChunk(chunk *Chunk) bool {
	indexChunk := chunk.head.indexChunk

	var pred *Chunk = terminal.inChunks.last
	var next *Chunk
	for pred != nil {
		if cmp := uint32Compare(indexChunk, pred.head.indexChunk); cmp == 0 {
			return false
		} else if cmp == 2 {
			log.SayInfo("chunk uint32Compare == 2 !!!")
			terminal.purgeChunkIn()
			pred = nil
			next = nil
			break
		} else if cmp > 0 {
			break
		}
		next = pred
		pred = pred.predChunk
	}

	terminal.inChunks.insertIn(pred, chunk, next)
	terminal.statistic.InChunkCount.Inc("")
	terminal.statistic.InChunkQueue.SetValueInt("", int64(terminal.inChunks.count))

	terminal.inClearChunks()
	return true
}

func (terminal *Terminal) inInsertPacket(chunk *Chunk) *Packet {
	indexPacket := chunk.head.indexPacket

	var pred *Packet = terminal.inPackets.first
	var next *Packet
	for pred != nil {
		if cmp := uint32Compare(indexPacket, pred.Index); cmp == 0 {
			break
		} else if cmp == 2 {
			log.SayInfo("packet uint32Compare == 2 !!!")
			terminal.purgePacketIn()
			pred = nil
			next = nil
			break
		} else if cmp > 0 {
			break
		}
		next = pred
		pred = pred.predPacket
	}

	packet := pred
	if packet == nil || packet.Index != indexPacket {
		packet = allocPacket()
		packet.Index = indexPacket
		packet.Channel = int(chunk.head.channel)
		packet.sizeData = chunk.head.sizePacket
		packet.collectData = 0
		packet.Data = make([]byte, chunk.head.sizePacket)
		terminal.inPackets.insertIn(pred, packet, next)
		terminal.statistic.InPacketCount.Inc("")
		terminal.statistic.InPacketQueue.SetValueInt("", int64(terminal.inPackets.count))
	}

	offset := chunk.head.offsetPacket
	size := uint32(chunk.head.sizeData)
	if chunk.head.sizePacket != packet.sizeData {
		log.SayInfo("ERROR: changed size of packet: %d != %d", chunk.head.sizePacket, packet.sizeData)
		return packet
	}
	if offset+size > packet.sizeData {
		log.SayInfo("ERROR: offset+size not in packet: %d+%d - %d", offset, size, packet.sizeData)
		return packet
	}

	copy(packet.Data[offset:offset+size], chunk.data[:size])
	packet.collectData += size

	return packet
}

func (terminal *Terminal) inReadyPacket(packet *Packet) bool {
	terminal.inPackets.extract(packet)
	terminal.statistic.InPacketQueue.SetValueInt("", int64(terminal.inPackets.count))

	terminal.pushReadyPacket(packet)
	return true
}

func (terminal *Terminal) inClearChunks() {
	now := time.Now()
	for {
		chunk := terminal.inChunks.first
		if chunk == nil || chunk.liveTo.After(now) {
			break
		}
		terminal.inChunks.extract(chunk)
		terminal.statistic.InChunkQueue.SetValueInt("", int64(terminal.inChunks.count))
		freeChunk(chunk)
	}
}
