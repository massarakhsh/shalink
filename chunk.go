package shalink

import (
	"encoding/binary"
	"sync"
	"time"
)

const dtgMaxSize int = 1420
const chunkHeadSize int = 16
const chunkInfoSize int = dtgMaxSize - chunkHeadSize
const offsetSigns uint32 = 0xFFFFFFF0
const offsetSignSynch uint32 = 0xFFFFFFFF

type Chunk struct {
	head chunkHead
	data [chunkInfoSize]uint8

	sizeData  int
	createdAt time.Time
	liveTo    time.Time

	client *Client
	packet *Packet
	pred   *Chunk
	next   *Chunk
}

type chunkHead struct {
	indexChunk   uint32
	indexPacket  uint32
	offsetPacket uint32
	sizePacket   uint32
}

type chunkPool struct {
	index   uint32
	channel chan *Chunk
	count   int
	first   *Chunk
	last    *Chunk
}

var chunkGate sync.Mutex
var chunkCountUsed int
var chunkCountFree int
var chunkFirstFree *Chunk

func allocChunk() *Chunk {
	// chunkGate.Lock()
	// defer chunkGate.Unlock()

	// chunk := chunkFirstFree
	// if chunk != nil {
	// 	chunkFirstFree = chunk.next
	// 	chunkCountFree--
	// 	*chunk = Chunk{}
	// } else {
	chunk := &Chunk{}
	// }
	// chunk.createdAt = time.Now()
	chunkCountUsed++
	return chunk
}

func (chunk *Chunk) Free() {
	// chunkGate.Lock()
	// defer chunkGate.Unlock()

	chunkCountUsed--
	// *chunk = Chunk{}
	// chunk.next = chunkFirstFree
	// chunkFirstFree = chunk
	// chunkCountFree++
}

func (chunk *Chunk) getSize() int {
	return chunkHeadSize + chunk.sizeData
}

func (chunk *Chunk) isData() bool {
	return chunk.head.offsetPacket < offsetSigns
}

func (chunk *Chunk) isSynch() bool {
	return chunk.head.offsetPacket == offsetSignSynch
}

func (pool *chunkPool) initialize() {
	pool.channel = make(chan *Chunk, maxInChunks)
}

func (pool *chunkPool) getCount() int {
	return pool.count + len(pool.channel)
}

func chunkFromBytes(data []byte) *Chunk {
	size := len(data)
	if size < chunkHeadSize {
		return nil
	}
	chunk := allocChunk()
	chunk.head.indexChunk = binary.BigEndian.Uint32(data[0:4])
	chunk.head.indexPacket = binary.BigEndian.Uint32(data[4:8])
	chunk.head.offsetPacket = binary.BigEndian.Uint32(data[8:12])
	chunk.head.sizePacket = binary.BigEndian.Uint32(data[12:16])
	chunk.sizeData = size - chunkHeadSize
	if chunk.sizeData > chunkInfoSize {
		chunk.Free()
		return nil
	}
	copy(chunk.data[:], data[chunkHeadSize:size])
	return chunk
}

func chunkToBytes(chunk *Chunk) []byte {
	data := []byte{}
	data = binary.BigEndian.AppendUint32(data, chunk.head.indexChunk)
	data = binary.BigEndian.AppendUint32(data, chunk.head.indexPacket)
	data = binary.BigEndian.AppendUint32(data, chunk.head.offsetPacket)
	data = binary.BigEndian.AppendUint32(data, chunk.head.sizePacket)
	if chunk.sizeData > 0 {
		data = append(data, chunk.data[:chunk.sizeData]...)
	}
	return data
}

func (pool *chunkPool) insertIn(pred *Chunk, chunk *Chunk, next *Chunk) {
	chunk.pred = pred
	if pred != nil {
		pred.next = chunk
	} else {
		pool.first = chunk
	}
	chunk.next = next
	if next != nil {
		next.pred = chunk
	} else {
		pool.last = chunk
	}
	pool.count++
}

func (pool *chunkPool) insertLast(chunk *Chunk) {
	pool.insertIn(pool.last, chunk, nil)
}

func (pool *chunkPool) extract(chunk *Chunk) {
	pred := chunk.pred
	next := chunk.next
	if pred != nil {
		pred.next = next
	} else {
		pool.first = next
	}
	if next != nil {
		next.pred = pred
	} else {
		pool.last = pred
	}
	pool.count--
}
