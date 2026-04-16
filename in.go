package shalink

import (
	"math/rand"
	"time"
)

func (terminal *Terminal) inChunk(chunk *Chunk) {
	terminal.statistic.InChunksTotal.Inc()
	terminal.statistic.InBytesTotal.Add(float64(chunk.getSize()))
	if chunk.isSynch() {
		terminal.inChunkSynch(chunk)
	} else if chunk.isData() {
		terminal.inChunkData(chunk)
	} else {
		chunk.Free()
	}
}

func (terminal *Terminal) inChunkData(chunk *Chunk) {
	terminal.gateIn.Lock()
	defer terminal.gateIn.Unlock()

	if !terminal.inProbeActual(chunk) {
		chunk.Free()
		return
	}
	chunk.liveTo = time.Now().Add(time.Second * 5)
	if !terminal.inInsertChunk(chunk) {
		chunk.Free()
		return
	}
	packet := terminal.inInsertPacket(chunk)
	if packet == nil {
		return
	}
	terminal.inControlPackets()
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
			terminal.SayLog("chunk uint32Compare == 2 !!!")
			terminal.purgeChunkIn()
			pred = nil
			next = nil
			break
		} else if cmp > 0 {
			break
		}
		next = pred
		pred = pred.pred
	}

	terminal.inChunks.insertIn(pred, chunk, next)
	terminal.statistic.InChunksData.Inc()
	terminal.statistic.InBytesData.Add(float64(chunk.sizeData))

	return true
}

func (terminal *Terminal) inInsertPacket(chunk *Chunk) *Packet {
	indexPacket := chunk.head.indexPacket

	var pred *Packet = terminal.inPackets.last
	var next *Packet = nil
	for pred != nil {
		if cmp := uint32Compare(indexPacket, pred.Index); cmp == 0 {
			break
		} else if cmp == 2 {
			terminal.SayLog("packet uint32Compare == 2 !!!")
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
		packet = &Packet{}
		packet.Index = indexPacket
		packet.sizeData = chunk.head.sizePacket
		packet.collectData = 0
		packet.Data = make([]byte, chunk.head.sizePacket)
		terminal.inPackets.insertIn(pred, packet, next)
		terminal.statistic.InPacketsCount.Inc()
	}

	chunk.packet = packet
	offset := chunk.head.offsetPacket
	size := uint32(chunk.sizeData)
	if chunk.head.sizePacket != packet.sizeData {
		terminal.SayLog("ERROR: changed size of packet: %d != %d", chunk.head.sizePacket, packet.sizeData)
		return packet
	}
	if offset+size > packet.sizeData {
		terminal.SayLog("ERROR: offset+size not in packet: %d+%d - %d", offset, size, packet.sizeData)
		return packet
	}

	copy(packet.Data[offset:offset+size], chunk.data[:size])
	packet.collectData += size
	if packet.collectData >= packet.sizeData {
		packet.isReady = true
		terminal.inControlPackets()
	}

	return packet
}

func (terminal *Terminal) inChunkSynch(chunk *Chunk) {
	if chunk.sizeData >= synchroHeadSize {
		synch := synchroFromBytes(chunk.data[:chunk.sizeData])
		if client := chunk.client; client != nil {
			client.synch = synch
		} else {
			terminal.synch = synch
		}
		if synch.holePairs > 0 {
			terminal.inSynchResend(synch)
		}
	}
	chunk.Free()
}

func (terminal *Terminal) inSynchResend(synch Synchro) {
	for r := 0; r < int(synch.holePairs); r++ {
		first := synch.holeList[r*2]
		count := synch.holeList[r*2+1]
		for c := uint32(0); c < count; c++ {
			indexChunk := first + c
			if chunk := terminal.outFoundChunk(indexChunk); chunk != nil {
				terminal.sendChunks([]*Chunk{chunk})
			}
		}
	}
}

func (terminal *Terminal) goInSynch() {
	terminal.gateIn.Lock()
	defer terminal.gateIn.Unlock()

	terminal.inControlChunks()
	terminal.inControlPackets()
	terminal.inControlSynch()
	terminal.nextInSynch = time.Now().Add(periodSynchro * 100 / time.Duration(90+rand.Intn(20)))
	terminal.isInSynch = false
}

func (terminal *Terminal) inControlChunks() {
	now := time.Now()
	for {
		chunk := terminal.inChunks.first
		if chunk == nil {
			break
		}
		if chunk.liveTo.After(now) && !chunk.packet.isLost && !chunk.packet.isReady {
			break
		}
		if packet := chunk.packet; packet != nil {
			packet.isLost = true
		}
		terminal.inChunks.extract(chunk)
		chunk.Free()
	}
}

func (terminal *Terminal) inControlPackets() {
	for {
		packet := terminal.inPackets.first
		if packet == nil {
			break
		}
		if packet.isReady {
			terminal.inPackets.extract(packet)
			terminal.pushReadyPacket(packet)
		} else if packet.isLost {
			terminal.inPackets.extract(packet)
		} else {
			break
		}
	}
}

func (terminal *Terminal) inControlSynch() {
	synch := terminal.inOutSynch()
	var indexNext uint32 = 0
	indexNeed := false
	for chunk := terminal.inChunks.first; chunk != nil; chunk = chunk.next {
		if !indexNeed {
			indexNeed = true
		} else if cmp := uint32Compare(chunk.head.indexChunk, indexNext); cmp > 0 {
			count := chunk.head.indexChunk - indexNext
			terminal.inAddHole(&synch, indexNext, count)
		}
		indexNext = chunk.head.indexChunk + 1
	}
	terminal.outSynchroSend(synch)
}

func (terminal *Terminal) inAddHole(synchro *Synchro, first uint32, count uint32) {
	if synchro.holePairs < maxHoleCount {
		synchro.holeList[synchro.holePairs*2] = first
		synchro.holeList[synchro.holePairs*2+1] = count
		synchro.holePairs++
	}
}

func (terminal *Terminal) inOutSynch() Synchro {
	var synch Synchro
	if count := terminal.outChunks.count; count > 0 {
		first := terminal.outChunks.first
		last := terminal.outChunks.last
		if first != nil && last != nil {
			indexFirst := first.head.indexChunk
			indexLast := last.head.indexChunk
			if uint32Compare(indexFirst, indexLast) <= 0 {
				synch.chunkOutCount = uint32(count)
				synch.chunkOutFirst = uint32(indexFirst)
				synch.chunkOutLast = uint32(indexLast)
			}
		}
	}
	return synch
}
