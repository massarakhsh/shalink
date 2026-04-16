package shalink

import (
	"sync"
	"time"
)

type ConfigTerminal struct {
	Latency  time.Duration
	IsLocal  bool
	IsMirror bool
}

type Terminal struct {
	Task
	config ConfigTerminal

	links     linkPool
	gateLinks sync.Mutex
	gateIn    sync.Mutex
	gateOut   sync.Mutex

	outPackets   packetPool
	inPackets    packetPool
	readyPackets packetPool

	outChunks chunkPool
	inChunks  chunkPool

	isOutSynch   bool
	nextOutSynch time.Time
	isInSynch    bool
	nextInSynch  time.Time

	synch     Synchro
	statistic Statistic
}

const maxOutPackets = 256
const maxInChunks = 1024
const maxReadyPackets = 256
const periodSynchro = time.Millisecond * 25

var dropFrequency = 0.1

func CreateTerminal(config ConfigTerminal) *Terminal {
	terminal := &Terminal{}
	terminal.config = config
	terminal.links.initialize()
	terminal.inPackets.initialize()
	terminal.outPackets.initialize()
	terminal.readyPackets.initialize()
	terminal.inChunks.initialize()
	terminal.outChunks.initialize()

	terminal.start()
	return terminal
}

func (terminal *Terminal) AddLink(config ConfigLink) *Link {
	return terminal.addLink(config)
}

func (terminal *Terminal) Stop() {
	terminal.gateLinks.Lock()
	defer terminal.gateLinks.Unlock()

	terminal.isStoping = true
	for link := terminal.links.first; link != nil; link = link.linkNext {
		link.stop()
	}
}

func (terminal *Terminal) SendPacket(data []byte) {
	packet := &Packet{}
	packet.Data = data
	terminal.pushOutPacket(packet)
	terminal.statistic.OutPacketsCount.Inc()
	terminal.statistic.DebugPackets++
}

func (terminal *Terminal) ProbePacket() *Packet {
	if len(terminal.readyPackets.channel) == 0 {
		return nil
	}
	return terminal.WaitPacket()
}

func (terminal *Terminal) WaitPacket() *Packet {
	packet, ok := <-terminal.readyPackets.channel
	if !ok {
		return nil
	}
	return packet
}

func (terminal *Terminal) start() {
	go terminal.goRun()
}

func (terminal *Terminal) goRun() {
	terminal.isStarted = true
	terminal.isOutSynch = false
	terminal.nextOutSynch = time.Now()
	for !terminal.isStoping {
		select {
		case packet, ok := <-terminal.outPackets.channel:
			if !ok {
				break
			}
			terminal.sendPacket(packet)
		case chunk, ok := <-terminal.inChunks.channel:
			if !ok {
				break
			}
			terminal.inChunk(chunk)
		case <-time.After(timeoutRun):
		}
		if !terminal.isOutSynch && time.Now().After(terminal.nextOutSynch) {
			terminal.isOutSynch = true
			go terminal.goOutSynch()
		}
		if !terminal.isInSynch && time.Now().After(terminal.nextInSynch) {
			terminal.isInSynch = true
			go terminal.goInSynch()
		}
	}
	terminal.isStoped = true
	terminal.SayLog("Terminal stopped")
}

func (terminal *Terminal) addLink(config ConfigLink) *Link {
	terminal.gateLinks.Lock()
	defer terminal.gateLinks.Unlock()

	link := createLink(terminal, config)
	terminal.links.insert(link)
	link.start()

	return link
}

func (terminal *Terminal) pushOutPacket(packet *Packet) {
	terminal.outPackets.pushOut(packet)
}

func (terminal *Terminal) pushReadyPacket(packet *Packet) {
	terminal.statistic.InPacketsReady.Inc()
	terminal.readyPackets.pushOut(packet)
}

func (terminal *Terminal) pushInChunk(chunk *Chunk) {
	terminal.inChunks.channel <- chunk
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
}

func (terminal *Terminal) sendChunks(chunks []*Chunk) {
	terminal.gateLinks.Lock()
	defer terminal.gateLinks.Unlock()

	for link := terminal.links.first; link != nil; link = link.linkNext {
		if terminal.config.IsLocal && link.config.IsServer {
			continue
		}
		link.pushChunks(chunks)
	}
}

func (terminal *Terminal) purgeChunkIn() {
	for {
		chunk := terminal.inChunks.first
		if chunk == nil {
			break
		}
		terminal.inChunks.extract(chunk)
		chunk.Free()
	}
}

func (terminal *Terminal) purgePacketIn() {
	for {
		packet := terminal.inPackets.first
		if packet == nil {
			break
		}
		terminal.inPackets.extract(packet)
	}
}

func (terminal *Terminal) SayLog(format string, args ...any) {
	terminal.statistic.sayLog(format, args...)
}
