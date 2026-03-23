package shalink

import (
	"sync"
	"time"

	"github.com/massarakhsh/lik/log"
)

type Terminal struct {
	Task

	packetGate      sync.Mutex
	packetIndexSend uint32

	linkGate  sync.Mutex
	linkFirst *Link
	linkLast  *Link

	chunkGate     sync.Mutex
	chunkIndex    uint32
	chunkOutFirst *chunkData
	chunkOutLast  *chunkData

	chanOut chan *sendPacket
	chanIn  chan *chunkData
}

func CreateTerminal() *Terminal {
	terminal := &Terminal{}
	terminal.chanOut = make(chan *sendPacket, 1024)
	terminal.chanIn = make(chan *chunkData, 1024)
	terminal.start()
	return terminal
}

func (terminal *Terminal) AddListener(address string) *Link {
	terminal.linkGate.Lock()
	defer terminal.linkGate.Unlock()

	link := createLink(address, true)
	terminal.insertLink(link)
	link.start()

	return link
}

func (terminal *Terminal) AddConnection(address string) *Link {
	terminal.linkGate.Lock()
	defer terminal.linkGate.Unlock()

	link := createLink(address, false)
	terminal.insertLink(link)
	link.start()

	return link
}

func (terminal *Terminal) Stop() {
	terminal.linkGate.Lock()
	defer terminal.linkGate.Unlock()

	terminal.isStoping = true
	for link := terminal.linkFirst; link != nil; link = link.linkNext {
		link.stop()
	}
}

func (terminal *Terminal) SendData(channel int, data []byte) {
	packet := &sendPacket{channel: channel, data: data}
	terminal.packetGate.Lock()
	packet.index = terminal.packetIndexSend
	terminal.packetIndexSend++
	terminal.packetGate.Unlock()
	terminal.pushOutPacket(packet)
}

func (terminal *Terminal) start() {
	go terminal.goRun()
}

func (terminal *Terminal) goRun() {
	terminal.isStarted = true
	for !terminal.isStoping {
		select {
		case packet, ok := <-terminal.chanOut:
			if !ok {
				break
			}
			terminal.sendPacket(packet)
		case chunk, ok := <-terminal.chanIn:
			if !ok {
				break
			}
			terminal.receiveChunk(chunk)
		case <-time.After(timeoutRun):
		}
	}
	terminal.isStoped = true
	log.SayInfo("Terminal stopped")
}

func (terminal *Terminal) pushOutPacket(packet *sendPacket) {
	terminal.chanOut <- packet
}

func (terminal *Terminal) pushInChunk(chunk *chunkData) {
	terminal.chanIn <- chunk
}

func (terminal *Terminal) sendPacket(packet *sendPacket) {
	chunks := terminal.packetChunking(packet)
	terminal.sendChunks(chunks)
}

func (terminal *Terminal) packetChunking(packet *sendPacket) []chunkData {
	terminal.chunkGate.Lock()
	defer terminal.chunkGate.Unlock()

	var chunks []chunkData
	offset := 0
	size := len(packet.data)
	for offset < size {
		chunkSize := size - offset
		if chunkSize > chunkInfoSize {
			chunkSize = chunkInfoSize
		}
		chunk := &chunkData{}
		chunk.head.bell = 0
		chunk.head.channel = uint8(packet.channel)
		chunk.head.indexPacket = packet.index
		chunk.head.sizeData = uint16(chunkSize)
		copy(chunk.data[:], packet.data[offset:offset+chunkSize])
		chunk.head.indexChunk = terminal.chunkIndex
		terminal.chunkIndex++
		chunks = append(chunks, *chunk)
		terminal.insertChunkOut(chunk)
		offset += chunkSize
	}

	return chunks
}

func (terminal *Terminal) sendChunks(chunks []chunkData) {
	terminal.linkGate.Lock()
	defer terminal.linkGate.Unlock()

	for link := terminal.linkFirst; link != nil; link = link.linkNext {
		link.pushChunks(chunks)
	}
}

func (terminal *Terminal) insertLink(link *Link) {
	link.terminal = terminal
	link.linkPred = terminal.linkLast
	if link.linkPred == nil {
		terminal.linkFirst = link
	} else {
		link.linkPred.linkNext = link
	}
	terminal.linkLast = link
}

func (terminal *Terminal) extractLink(link *Link) {
	pred := link.linkPred
	next := link.linkNext
	if pred == nil {
		terminal.linkFirst = next
	} else {
		pred.linkNext = next
	}
	if next == nil {
		terminal.linkLast = pred
	} else {
		next.linkPred = pred
	}
	link.terminal = nil
}

func (terminal *Terminal) insertChunkOut(chunk *chunkData) {
	chunk.predChunk = terminal.chunkOutLast
	if chunk.predChunk == nil {
		terminal.chunkOutFirst = chunk
	} else {
		chunk.predChunk.nextChunk = chunk
	}
	terminal.chunkOutLast = chunk
}

func (terminal *Terminal) extractChunkOut(chunk *chunkData) {
	pred := chunk.predChunk
	next := chunk.nextChunk
	if pred == nil {
		terminal.chunkOutFirst = next
	} else {
		pred.nextChunk = next
	}
	if next == nil {
		terminal.chunkOutLast = pred
	} else {
		next.predChunk = pred
	}
	chunk.predChunk = nil
}

func (terminal *Terminal) receiveChunk(chunk *chunkData) {
}
