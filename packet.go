package shalink

import (
	"time"
)

type Packet struct {
	Index uint32
	Data  []byte

	latency     time.Duration
	sizeData    uint32
	collectData uint32
	isLost      bool
	isReady     bool
	nextPacket  *Packet
	predPacket  *Packet
}

type packetPool struct {
	index   uint32
	channel chan *Packet
	count   int
	first   *Packet
	last    *Packet
}

func (pool *packetPool) initialize() {
	pool.channel = make(chan *Packet, maxOutPackets)
}

func (pool *packetPool) getCount() int {
	return pool.count + len(pool.channel)
}

func (pool *packetPool) pushOut(packet *Packet) {
	packet.Index = pool.index
	pool.index++
	pool.channel <- packet
}

func (pool *packetPool) insertIn(pred *Packet, packet *Packet, next *Packet) {
	packet.predPacket = pred
	if pred != nil {
		pred.nextPacket = packet
	} else {
		pool.first = packet
	}
	packet.nextPacket = next
	if next != nil {
		next.predPacket = packet
	} else {
		pool.last = packet
	}
	pool.count++
}

func (pool *packetPool) extract(packet *Packet) {
	pred := packet.predPacket
	next := packet.nextPacket
	if pred != nil {
		pred.nextPacket = next
	} else {
		pool.first = next
	}
	if next != nil {
		next.predPacket = pred
	} else {
		pool.last = pred
	}
	pool.count--
}
