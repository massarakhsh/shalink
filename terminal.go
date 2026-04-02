package shalink

import (
	"sync"
	"time"

	"github.com/massarakhsh/lik/log"
)

type ConfigTerminal struct {
	Latency  time.Duration
	IsMirror bool
}

type Terminal struct {
	Task
	config ConfigTerminal

	linkGate  sync.Mutex
	linkFirst *Link
	linkLast  *Link

	outPacketGate  sync.Mutex
	outPacketIndex uint32
	outPacketChan  chan *Packet

	outChunkGate  sync.Mutex
	outChunkIndex uint32
	outChunks     PoolChunk
	outChunkTo    time.Time

	inChunkChan chan *Chunk
	inChunkGate sync.Mutex
	inChunks    PoolChunk

	inPackets PoolPacket

	readyPacketChan chan *Packet

	statistic Statistic
}

const maxOutPackets = 256
const maxInChunks = 1024
const maxReadyPackets = 256

func CreateTerminal(config ConfigTerminal) *Terminal {
	terminal := &Terminal{}
	terminal.config = config
	terminal.outPacketChan = make(chan *Packet, maxOutPackets)
	terminal.inChunkChan = make(chan *Chunk, maxInChunks)
	terminal.readyPacketChan = make(chan *Packet, maxReadyPackets)
	terminal.start()
	return terminal
}

func (terminal *Terminal) AddLink(config ConfigLink) *Link {
	return terminal.addLink(config)
}

func (terminal *Terminal) Stop() {
	terminal.linkGate.Lock()
	defer terminal.linkGate.Unlock()

	terminal.isStoping = true
	for link := terminal.linkFirst; link != nil; link = link.linkNext {
		link.stop()
	}
}

func (terminal *Terminal) SendPacket(channel int, data []byte) {
	packet := allocPacket()
	packet.Channel = channel
	packet.Data = data
	terminal.pushOutPacket(packet)
	terminal.statistic.OutPacketCount.Inc("")
}

func (terminal *Terminal) ProbePacket() *Packet {
	if len(terminal.readyPacketChan) == 0 {
		return nil
	}
	return terminal.WaitPacket()
}

func (terminal *Terminal) WaitPacket() *Packet {
	select {
	case packet, ok := <-terminal.readyPacketChan:
		if !ok {
			return nil
		}
		return packet
	}
}

func (terminal *Terminal) start() {
	go terminal.goRun()
}

func (terminal *Terminal) goRun() {
	terminal.isStarted = true
	for !terminal.isStoping {
		select {
		case packet, ok := <-terminal.outPacketChan:
			if !ok {
				break
			}
			terminal.sendPacket(packet)
		case chunk, ok := <-terminal.inChunkChan:
			if !ok {
				break
			}
			terminal.inChunk(chunk)
		case <-time.After(timeoutRun):
		}
	}
	terminal.isStoped = true
	log.SayInfo("Terminal stopped")
}

func (terminal *Terminal) addLink(config ConfigLink) *Link {
	terminal.linkGate.Lock()
	defer terminal.linkGate.Unlock()

	link := createLink(config)
	terminal.insertLink(link)
	link.start()

	return link
}

func (terminal *Terminal) pushOutPacket(packet *Packet) {
	terminal.outPacketGate.Lock()
	defer terminal.outPacketGate.Unlock()

	packet.Index = terminal.outPacketIndex
	terminal.outPacketIndex++
	terminal.outPacketChan <- packet
}

func (terminal *Terminal) pushReadyPacket(packet *Packet) {
	terminal.statistic.InPacketReady.Inc("")
	terminal.readyPacketChan <- packet
}

func (terminal *Terminal) pushInChunk(chunk *Chunk) {
	terminal.inChunkChan <- chunk
}

func (terminal *Terminal) getLatency(packet *Packet) time.Duration {
	if packet.latency != 0 {
		return packet.latency
	}
	return terminal.config.Latency
}

func (terminal *Terminal) sendPacket(packet *Packet) {
	chunks := terminal.outChunking(packet)
	terminal.sendChunks(chunks)
	terminal.statistic.OutChunkCount.Add("", float64(len(chunks)))
}

func (terminal *Terminal) sendChunks(chunks []Chunk) {
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

func (terminal *Terminal) purgeChunkIn() {
	for {
		chunk := terminal.inChunks.first
		if chunk == nil {
			break
		}
		terminal.inChunks.extract(chunk)
		freeChunk(chunk)
	}
	terminal.statistic.InChunkQueue.SetValueInt("", int64(terminal.inChunks.count))
}

func (terminal *Terminal) purgePacketIn() {
	for {
		packet := terminal.inPackets.first
		if packet == nil {
			break
		}
		terminal.inPackets.extract(packet)
		freePacket(packet)
	}
	terminal.statistic.InPacketQueue.SetValueInt("", int64(terminal.inPackets.count))
}
