package shalink

import "time"

type Packet struct {
	Channel int
	Index   uint32
	Data    []byte

	latency     time.Duration
	sizeData    uint32
	collectData uint32
	nextPacket  *Packet
	predPacket  *Packet
}

type PoolPacket struct {
	count int
	first *Packet
	last  *Packet
}

func allocPacket() *Packet {
	packet := &Packet{}
	if packet == nil {
		panic("packet do not allocated")
	}
	return packet
}

func freePacket(packet *Packet) {
}

func (pool *PoolPacket) insertIn(pred *Packet, packet *Packet, next *Packet) {
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

func (pool *PoolPacket) extract(packet *Packet) {
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
